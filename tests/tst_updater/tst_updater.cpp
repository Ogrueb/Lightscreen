// M13: the update check reads the latest GitHub release of UPDATE_REPO (version.pri) and compares its tag
// with the running version. The network is a stub; nothing here reaches GitHub.
#include <QtTest>
#include <cstring>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

#include <tools/uploader/uploader.h>
#include <updater/updater.h>

// A reply whose body, HTTP status and error the test controls.
class CannedReply : public QNetworkReply
{
public:
    CannedReply(QNetworkAccessManager::Operation operation, const QNetworkRequest &request, QObject *parent)
        : QNetworkReply(parent), mOffset(0)
    {
        setOperation(operation);
        setRequest(request);
        setUrl(request.url());
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    }

    void finish(int status, const QByteArray &body, NetworkError networkError = NoError)
    {
        mBody = body;
        mOffset = 0;

        if (status > 0) {
            setAttribute(QNetworkRequest::HttpStatusCodeAttribute, status);
        }

        if (networkError != NoError) {
            setError(networkError, QStringLiteral("canned error"));
        }

        setFinished(true);
        emit finished();
    }

    void abort() override
    {
        if (isFinished()) {
            return;
        }

        setError(OperationCanceledError, QStringLiteral("Operation canceled"));
        setFinished(true);
        emit finished();
    }

    qint64 bytesAvailable() const override
    {
        return mBody.size() - mOffset + QIODevice::bytesAvailable();
    }

    bool isSequential() const override
    {
        return true;
    }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        const qint64 count = qMin<qint64>(maxSize, mBody.size() - mOffset);

        if (count <= 0) {
            return -1;
        }

        memcpy(data, mBody.constData() + mOffset, count);
        mOffset += count;
        return count;
    }

private:
    QByteArray mBody;
    qint64 mOffset;
};

class CannedNetwork : public QNetworkAccessManager
{
public:
    using QNetworkAccessManager::QNetworkAccessManager;

    QList<QPointer<CannedReply>> replies;

protected:
    QNetworkReply *createRequest(Operation operation, const QNetworkRequest &request, QIODevice *outgoingData) override
    {
        Q_UNUSED(outgoingData);
        auto reply = new CannedReply(operation, request, this);
        replies.append(reply);
        return reply;
    }
};

class tst_Updater : public QObject
{
    Q_OBJECT

private:
    Uploader *mUploader = nullptr;
    CannedNetwork *mNetwork = nullptr;
    Updater *mUpdater = nullptr;
    QStringList mEvents; // "done:<newer>|<htmlUrl>" and "failed:<message>", in emission order

    static QByteArray release(const QString &tag, const QString &htmlUrl = QStringLiteral("https://github.com/owner/repo/releases/tag/x"))
    {
        QJsonObject object;
        object.insert(QStringLiteral("tag_name"), tag);
        object.insert(QStringLiteral("html_url"), htmlUrl);
        object.insert(QStringLiteral("name"), QStringLiteral("Release ") + tag);
        return QJsonDocument(object).toJson();
    }

    // Runs one check and answers it with the given reply.
    void answer(int status, const QByteArray &body, QNetworkReply::NetworkError error = QNetworkReply::NoError)
    {
        mUpdater->check();
        QCOMPARE(mNetwork->replies.size(), 1);
        QVERIFY(mNetwork->replies.at(0));
        mNetwork->replies.at(0)->finish(status, body, error);
    }

private slots:
    void initTestCase()
    {
        QCoreApplication::setApplicationVersion(QStringLiteral("2.5"));
    }

    void init()
    {
        mUploader = new Uploader;
        mNetwork = new CannedNetwork;
        Uploader::setInstance(mUploader);
        mUploader->setNetwork(mNetwork);

        mUpdater = new Updater;
        mEvents.clear();
        connect(mUpdater, &Updater::done, this, [this](bool newer, const QString &htmlUrl) {
            mEvents << QStringLiteral("done:%1|%2").arg(newer ? "true" : "false", htmlUrl);
        });
        connect(mUpdater, &Updater::failed, this, [this](const QString &message) {
            mEvents << QStringLiteral("failed:") + message;
        });
    }

    void cleanup()
    {
        delete mUpdater;
        mUpdater = nullptr;
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        Uploader::setInstance(nullptr);
        delete mNetwork;
        delete mUploader;
        mNetwork = nullptr;
        mUploader = nullptr;
    }

    void urls()
    {
        QVERIFY(!Updater::repository().isEmpty());
        QCOMPARE(Updater::latestReleaseApiUrl(), QUrl("https://api.github.com/repos/" + Updater::repository() + "/releases/latest"));
        QCOMPARE(Updater::releasesPageUrl(), QUrl("https://github.com/" + Updater::repository() + "/releases"));
    }

    void request()
    {
        mUpdater->check();
        QCOMPARE(mNetwork->replies.size(), 1);

        const QNetworkRequest request = mNetwork->replies.at(0)->request();
        QCOMPARE(mNetwork->replies.at(0)->operation(), QNetworkAccessManager::GetOperation);
        QCOMPARE(request.url(), Updater::latestReleaseApiUrl());
        QCOMPARE(request.rawHeader("Accept"), QByteArray("application/vnd.github+json"));
        QCOMPARE(request.rawHeader("User-Agent"), QByteArray("Lightscreen/2.5"));
        QVERIFY(mEvents.isEmpty());
    }

    // A second check while one is running does not send another request.
    void checkWhileRunning()
    {
        mUpdater->check();
        mUpdater->check();
        QCOMPARE(mNetwork->replies.size(), 1);
    }

    void tag_data()
    {
        QTest::addColumn<QString>("tag");
        QTest::addColumn<bool>("newer");

        QTest::newRow("newer minor") << "2.6" << true;
        QTest::newRow("newer patch") << "2.5.1" << true;
        QTest::newRow("newer major") << "3.0" << true;
        QTest::newRow("equal") << "2.5" << false;
        QTest::newRow("equal with trailing zero") << "2.5.0" << false;
        QTest::newRow("older") << "2.4.9" << false;
        QTest::newRow("v prefix newer") << "v2.6" << true;
        QTest::newRow("v prefix equal") << "v2.5" << false;
        QTest::newRow("V prefix newer") << "V2.6.0" << true;
        QTest::newRow("suffix ignored") << "v2.6.0-beta" << true;
    }

    void tag()
    {
        QFETCH(QString, tag);
        QFETCH(bool, newer);

        answer(200, release(tag, "https://github.com/owner/repo/releases/tag/" + tag));

        const QStringList expected { QStringLiteral("done:%1|https://github.com/owner/repo/releases/tag/%2").arg(newer ? "true" : "false", tag) };
        QCOMPARE(mEvents.size(), expected.size());
        QVERIFY(std::equal(mEvents.cbegin(), mEvents.cend(), expected.cbegin()));
    }

    // A release without html_url still gives the user somewhere to go.
    void missingHtmlUrl()
    {
        answer(200, "{\"tag_name\": \"v9.0\"}");

        QCOMPARE(mEvents.size(), 1);
        QCOMPARE(mEvents.at(0), QStringLiteral("done:true|") + Updater::releasesPageUrl().toString());
    }

    // GitHub answers 404 on releases/latest while the repository has no published release.
    void notFound()
    {
        answer(404, "{\"message\": \"Not Found\"}", QNetworkReply::ContentNotFoundError);

        QCOMPARE(mEvents.size(), 1);
        QVERIFY(mEvents.at(0).startsWith("failed:"));
        QVERIFY(mEvents.at(0).contains(Updater::releasesPageUrl().toString()));
    }

    void serverError()
    {
        answer(500, "oops", QNetworkReply::InternalServerError);

        QCOMPARE(mEvents.size(), 1);
        QCOMPARE(mEvents.at(0), QStringLiteral("failed:canned error"));
    }

    // A non-200 success status (no network error) is not a release either.
    void unexpectedStatus()
    {
        answer(204, "");

        QCOMPARE(mEvents.size(), 1);
        QVERIFY(mEvents.at(0).startsWith("failed:"));
        QVERIFY(mEvents.at(0).contains("204"));
    }

    void networkError()
    {
        answer(0, "", QNetworkReply::HostNotFoundError);

        QCOMPARE(mEvents.size(), 1);
        QCOMPARE(mEvents.at(0), QStringLiteral("failed:canned error"));
    }

    void malformed_data()
    {
        QTest::addColumn<QByteArray>("body");

        QTest::newRow("not json") << QByteArray("<html>rate limited</html>");
        QTest::newRow("empty") << QByteArray();
        QTest::newRow("array") << QByteArray("[]");
        QTest::newRow("no tag_name") << QByteArray("{\"html_url\": \"https://github.com/owner/repo\"}");
        QTest::newRow("tag_name not a string") << QByteArray("{\"tag_name\": 3}");
        QTest::newRow("tag_name not a version") << QByteArray("{\"tag_name\": \"latest\"}");
    }

    // The old updater read a failed or garbled reply as version 0 and said "up to date" (bug 6).
    void malformed()
    {
        QFETCH(QByteArray, body);

        answer(200, body);

        QCOMPARE(mEvents.size(), 1);
        QVERIFY(mEvents.at(0).startsWith("failed:"));
    }

    // After an answer the updater can check again.
    void checkAgain()
    {
        answer(200, release("2.4"));
        mUpdater->check();
        QCOMPARE(mNetwork->replies.size(), 2);
        mNetwork->replies.at(1)->finish(200, release("2.6"));

        QCOMPARE(mEvents.size(), 2);
        QVERIFY(mEvents.at(0).startsWith("done:false|"));
        QVERIFY(mEvents.at(1).startsWith("done:true|"));
    }

    // Closing the "Check now" dialog destroys the updater with the request still running.
    void destroyedWhileRunning()
    {
        mUpdater->check();
        QPointer<CannedReply> reply = mNetwork->replies.at(0);

        delete mUpdater;
        mUpdater = nullptr;

        QVERIFY(mEvents.isEmpty());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(reply.isNull());
    }
};

QTEST_MAIN(tst_Updater)
#include "tst_updater.moc"
