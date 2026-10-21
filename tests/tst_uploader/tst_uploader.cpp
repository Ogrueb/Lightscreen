// Stage 4 safety net: the uploader core (tools/uploader) and, lightly, the pomf uploader.
// Records current behaviour, not intended behaviour: assertions marked "Current behaviour" document quirks
// a later step may change on purpose. The pomf assertions are S03's baseline. Since M12 pomf is the only service.
#include <QtTest>
#include <cstring>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include <tools/screenshot.h>
#include <tools/screenshotmanager.h>
#include <tools/uploader/imageuploader.h>
#include <tools/uploader/pomfuploader.h>
#include <tools/uploader/uploader.h>

// A reply whose body, error and progress the test controls.
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

    QByteArray sentBody;

    void finish(const QByteArray &body, NetworkError networkError = NoError)
    {
        mBody = body;
        mOffset = 0;

        if (networkError != NoError) {
            setError(networkError, QStringLiteral("canned error"));
        }

        setFinished(true);
        emit finished();
    }

    void sendProgress(qint64 bytesSent, qint64 bytesTotal)
    {
        emit uploadProgress(bytesSent, bytesTotal);
    }

    // Like QNetworkReplyHttpImpl::abort: an unfinished reply gets OperationCanceledError and emits finished at once.
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

// Never touches the network: every request gets a CannedReply, kept for the test to inspect and finish.
class CannedNetwork : public QNetworkAccessManager
{
public:
    using QNetworkAccessManager::QNetworkAccessManager;

    QList<QPointer<CannedReply>> replies;

protected:
    QNetworkReply *createRequest(Operation operation, const QNetworkRequest &request, QIODevice *outgoingData) override
    {
        auto reply = new CannedReply(operation, request, this);

        if (outgoingData) {
            reply->sentBody = outgoingData->readAll();
        }

        replies.append(reply);
        return reply;
    }
};

// Exposes the protected per-instance settings hash.
class PomfProbe : public PomfUploader
{
public:
    using PomfUploader::PomfUploader;

    QVariantHash &settingsHash()
    {
        return mSettings;
    }
};

class tst_Uploader : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir *mDir = nullptr;
    QSettings *mSettings = nullptr;
    Uploader *mUploader = nullptr; // owned by mManager
    CannedNetwork *mNetwork = nullptr;
    ScreenshotManager *mManager = nullptr;
    QStringList mEvents; // "error:<text>" and "done:<file>|<url>|<deleteHash>", in emission order

    QString writeFile(const QString &name)
    {
        QFile file(mDir->filePath(name));

        if (!file.open(QIODevice::WriteOnly)) {
            return QString();
        }

        file.write("not really an image");
        return file.fileName();
    }

    static QString doneEvent(const QString &fileName, const QString &url = QString(), const QString &deleteHash = QString())
    {
        return QStringLiteral("done:%1|%2|%3").arg(fileName, url, deleteHash);
    }

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings::setDefaultFormat(QSettings::IniFormat);
    }

    void init()
    {
        mDir = new QTemporaryDir;
        QVERIFY(mDir->isValid());

        mSettings = new QSettings(mDir->filePath("settings.ini"), QSettings::IniFormat);
        mSettings->setValue("upload/pomf/pomf_url", "https://pomf.example");
        mSettings->setValue("options/history", false); // The manager writes history on Uploader::done; not tested here.

        mManager = new ScreenshotManager(mSettings, mDir->path() + QDir::separator());
        ScreenshotManager::setInstance(mManager);

        mNetwork = new CannedNetwork;
        mUploader = mManager->uploader();
        mUploader->setNetwork(mNetwork);

        mEvents.clear();
        connect(mUploader, &Uploader::error, this, [this](const QString &errorString) {
            mEvents << QStringLiteral("error:") + errorString;
        });
        connect(mUploader, &Uploader::done, this, [this](const QString &fileName, const QString &url, const QString &deleteHash) {
            mEvents << doneEvent(fileName, url, deleteHash);
        });
    }

    void cleanup()
    {
        // Run pending deleteLater() calls on the uploaders before their network goes away.
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        ScreenshotManager::setInstance(nullptr);
        delete mManager;
        delete mNetwork;
        delete mSettings;
        delete mDir;
        mManager = nullptr;
        mUploader = nullptr;
        mNetwork = nullptr;
        mSettings = nullptr;
        mDir = nullptr;
    }

    void serviceName_data()
    {
        QTest::addColumn<int>("index");
        QTest::addColumn<QString>("name");

        // M12: index 0 was Imgur and 1 was pomf; every index now maps to pomf, so an old Imgur or pomf setting both work.
        QTest::newRow("0") << 0 << "pomf";
        QTest::newRow("1 (pomf before M12)") << 1 << "pomf";
        QTest::newRow("2") << 2 << "pomf";
        QTest::newRow("-1") << -1 << "pomf";
    }

    void serviceName()
    {
        QFETCH(int, index);
        QFETCH(QString, name);
        QCOMPARE(Uploader::serviceName(index), name);
    }

    void factory_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("className");

        // M12: names the factory does not know fall back to pomf instead of returning nullptr (bug 21).
        QTest::newRow("pomf") << "pomf" << "PomfUploader";
        QTest::newRow("removed service") << "imgur" << "PomfUploader";
        QTest::newRow("case") << "Pomf" << "PomfUploader";
        QTest::newRow("empty") << "" << "PomfUploader";
        QTest::newRow("unknown") << "custom" << "PomfUploader";
    }

    void factory()
    {
        QFETCH(QString, name);
        QFETCH(QString, className);

        QScopedPointer<ImageUploader> uploader(ImageUploader::factory(name, mSettings, mNetwork));

        if (className.isEmpty()) {
            QVERIFY(uploader.isNull());
        } else {
            QVERIFY(!uploader.isNull());
            QCOMPARE(QString(uploader->metaObject()->className()), className);
        }
    }

    void settingsRoundTrip()
    {
        mSettings->setValue("upload/pomf/kept", "untouched");
        mSettings->setValue("upload/pomf/group/nested", "ignored");

        QVariantHash saved;
        saved["pomf_url"] = "https://other.example";
        saved["number"] = 42;
        ImageUploader::saveSettings(mSettings, "pomf", saved);

        QCOMPARE(mSettings->value("upload/pomf/pomf_url").toString(), QStringLiteral("https://other.example"));

        QVariantHash expected = saved;
        expected["kept"] = "untouched"; // saveSettings merges: keys missing from the hash are left in place.
        // loadSettings reads child keys only, so "group/nested" is not returned.
        QCOMPARE(ImageUploader::loadSettings(mSettings, "pomf"), expected);

        QVERIFY(ImageUploader::loadSettings(mSettings, "nothing-here").isEmpty());

        // Current behaviour: once written to the INI file and read back, numbers come back as strings.
        mSettings->sync();
        QSettings reopened(mSettings->fileName(), QSettings::IniFormat);
        const QVariantHash reloaded = ImageUploader::loadSettings(&reopened, "pomf");

        QCOMPARE(reloaded["number"], QVariant(QStringLiteral("42")));
        QCOMPARE(reloaded["pomf_url"], QVariant(QStringLiteral("https://other.example")));
    }

    void instanceSettingsRoundTrip()
    {
        PomfProbe probe(mSettings, mNetwork); // The constructor loads "upload/pomf".
        QCOMPARE(probe.settingsHash().value("pomf_url").toString(), QStringLiteral("https://pomf.example"));

        probe.settingsHash()["pomf_url"] = "https://changed.example";
        probe.saveSettings();
        QCOMPARE(mSettings->value("upload/pomf/pomf_url").toString(), QStringLiteral("https://changed.example"));

        mSettings->setValue("upload/pomf/pomf_url", "https://external.example");
        probe.loadSettings();
        QCOMPARE(probe.settingsHash().value("pomf_url").toString(), QStringLiteral("https://external.example"));
    }

    void uploadEmptyFileNameDoesNothing()
    {
        mUploader->upload(QString(), "pomf");

        QCOMPARE(mUploader->uploading(), 0);
        QVERIFY(mNetwork->replies.isEmpty());
        QVERIFY(mEvents.isEmpty());
    }

    void uploadDone()
    {
        const QString file = writeFile("shot.png");
        QSignalSpy progressSpy(mUploader, &Uploader::progressChanged);

        mUploader->upload(file, "pomf");

        QCOMPARE(mUploader->uploading(), 1);
        QCOMPARE(mNetwork->replies.size(), 1);
        CannedReply *reply = mNetwork->replies.first();
        QCOMPARE(reply->operation(), QNetworkAccessManager::PostOperation);
        QCOMPARE(reply->url(), QUrl("https://pomf.example/upload.php"));
        QVERIFY(reply->request().header(QNetworkRequest::ContentTypeHeader).toString().startsWith("multipart/form-data; boundary="));
        // Header names are compared case-insensitively: Qt 6.7 and later send them in lower case.
        const QByteArray body = reply->sentBody.toLower();
        QVERIFY2(body.contains("content-disposition: form-data; name=\"files[]\"; filename=\"shot.png\""), reply->sentBody.constData());
        QVERIFY2(body.contains("content-type: image/png"), reply->sentBody.constData());
        QVERIFY(reply->sentBody.contains("not really an image"));

        reply->finish("{\"success\": true, \"files\": [{\"url\": \"https://pomf.example/abc.png\"}]}");

        QCOMPARE(mEvents, QStringList() << doneEvent(file, "https://pomf.example/abc.png"));
        QCOMPARE(mUploader->lastUrl(), QStringLiteral("https://pomf.example/abc.png"));
        QCOMPARE(mUploader->uploading(), 0);
        QCOMPARE(mUploader->progress(), 0);
        QCOMPARE(progressSpy.count(), 0);
    }

    void pomfResponse_data()
    {
        QTest::addColumn<QByteArray>("body");
        QTest::addColumn<int>("networkError");
        QTest::addColumn<QStringList>("events"); // with "%f" standing for the file name

        const int noError = QNetworkReply::NoError;

        QTest::newRow("success")
                << QByteArray("{\"success\": true, \"files\": [{\"url\": \"https://x/1.png\"}, {\"url\": \"https://x/2.png\"}]}")
                << noError << QStringList{ "done:%f|https://x/1.png|" };
        // Current behaviour: success with no files is "done" with an empty URL, and no error.
        QTest::newRow("success, empty files")
                << QByteArray("{\"success\": true, \"files\": []}")
                << noError << QStringList{ "done:%f||" };
        QTest::newRow("failure with description")
                << QByteArray("{\"success\": false, \"files\": [], \"description\": \"File too big\"}")
                << noError << QStringList{ "error:File too big", "done:%f||" };
        QTest::newRow("failure without description")
                << QByteArray("{\"success\": false, \"files\": []}")
                << noError << QStringList{ "error:Host error", "done:%f||" };
        // Current behaviour: "success" must be a JSON boolean; the string "true" is a failure.
        QTest::newRow("success as a string")
                << QByteArray("{\"success\": \"true\", \"files\": [{\"url\": \"https://x/1.png\"}]}")
                << noError << QStringList{ "error:Host error", "done:%f||" };
        QTest::newRow("no files key")
                << QByteArray("{\"success\": true}")
                << noError << QStringList{ "error:Invalid response from uploader", "done:%f||" };
        QTest::newRow("not JSON")
                << QByteArray("<html>502 Bad Gateway</html>")
                << noError << QStringList{ "error:Invalid response from uploader", "done:%f||" };
        QTest::newRow("network error, empty body")
                << QByteArray()
                << int(QNetworkReply::HostNotFoundError) << QStringList{ "error:Error reaching uploader", "done:%f||" };
        // Current behaviour: a network or HTTP error is ignored when the body is a JSON object.
        QTest::newRow("HTTP error with a success body")
                << QByteArray("{\"success\": true, \"files\": [{\"url\": \"https://x/1.png\"}]}")
                << int(QNetworkReply::InternalServerError) << QStringList{ "done:%f|https://x/1.png|" };
        QTest::newRow("HTTP error with a failure body")
                << QByteArray("{\"success\": false, \"files\": [], \"description\": \"Server on fire\"}")
                << int(QNetworkReply::InternalServerError) << QStringList{ "error:Server on fire", "done:%f||" };
    }

    void pomfResponse()
    {
        QFETCH(QByteArray, body);
        QFETCH(int, networkError);
        QFETCH(QStringList, events);

        const QString file = writeFile("shot.png");
        mUploader->upload(file, "pomf");
        QCOMPARE(mNetwork->replies.size(), 1);

        mNetwork->replies.first()->finish(body, QNetworkReply::NetworkError(networkError));

        events.replaceInStrings("%f", file);
        QCOMPARE(mEvents, events);
        QCOMPARE(mUploader->uploading(), 0);
    }

    // The pomf uploader fails inside upload(), before any request is made.
    void pomfErrorBeforeRequest_data()
    {
        QTest::addColumn<QString>("pomfUrl");
        QTest::addColumn<bool>("fileExists");
        QTest::addColumn<QString>("errorString");

        QTest::newRow("no pomf URL") << QString() << true << "Invalid pomf uploader URL!";
        QTest::newRow("unreadable file") << "https://pomf.example" << false << "Unable to read screenshot file";
    }

    void pomfErrorBeforeRequest()
    {
        QFETCH(QString, pomfUrl);
        QFETCH(bool, fileExists);
        QFETCH(QString, errorString);

        mSettings->setValue("upload/pomf/pomf_url", pomfUrl);
        const QString file = fileExists ? writeFile("shot.png") : mDir->filePath("missing.png");

        mUploader->upload(file, "pomf");

        QVERIFY(mNetwork->replies.isEmpty());
        QCOMPARE(mEvents, QStringList() << "error:" + errorString << doneEvent(file));
        QCOMPARE(mUploader->uploading(), 0);
    }

    // Bug 21 (fixed in M12): an unknown service name used to reach a null uploader and crash. It now goes to pomf,
    // which with no pomf URL set reports an error and finishes the upload.
    void uploadUnknownService()
    {
        mSettings->setValue("upload/pomf/pomf_url", QString());
        const QString file = writeFile("shot.png");

        mUploader->upload(file, "unknown");

        QVERIFY(mNetwork->replies.isEmpty());
        QCOMPARE(mEvents, QStringList() << "error:Invalid pomf uploader URL!" << doneEvent(file));
        QCOMPARE(mUploader->uploading(), 0);
    }

    void progressOverTwoUploads()
    {
        const QString first = writeFile("first.png");
        const QString second = writeFile("second.png");
        QSignalSpy progressSpy(mUploader, &Uploader::progressChanged);

        mUploader->upload(first, "pomf");
        mUploader->upload(second, "pomf");
        QCOMPARE(mUploader->uploading(), 2);
        QCOMPARE(mNetwork->replies.size(), 2);
        CannedReply *firstReply = mNetwork->replies.at(0);
        CannedReply *secondReply = mNetwork->replies.at(1);

        // Each uploader rounds bytesSent / bytesTotal to a percentage; the Uploader averages them with integer division.
        firstReply->sendProgress(33, 100);
        QCOMPARE(progressSpy.count(), 1);
        QCOMPARE(progressSpy.last().at(0).toInt(), 16);
        QCOMPARE(mUploader->progress(), 16);

        secondReply->sendProgress(1, 3);
        QCOMPARE(progressSpy.last().at(0).toInt(), 33); // (33 + 33) / 2

        secondReply->sendProgress(3, 3);
        QCOMPARE(progressSpy.last().at(0).toInt(), 66); // (33 + 100) / 2

        // Same percentage again: ImageUploader::setProgress does not re-emit, so neither does the Uploader.
        secondReply->sendProgress(30, 30);
        QCOMPARE(progressSpy.count(), 3);

        secondReply->finish("{\"success\": true, \"files\": [{\"url\": \"https://x/2.png\"}]}");
        QCOMPARE(mUploader->uploading(), 1);
        QCOMPARE(mUploader->progress(), 66); // Not recomputed when one of two finishes.

        firstReply->finish("{\"success\": true, \"files\": [{\"url\": \"https://x/1.png\"}]}");
        QCOMPARE(mUploader->uploading(), 0);
        QCOMPARE(mUploader->progress(), 0);
        QCOMPARE(progressSpy.count(), 3); // Current behaviour: the reset to 0 is not signalled.

        QCOMPARE(mEvents, QStringList() << doneEvent(second, "https://x/2.png") << doneEvent(first, "https://x/1.png"));
        QCOMPARE(mUploader->lastUrl(), QStringLiteral("https://x/1.png"));
    }

    void errorResetsProgressOfAllUploads()
    {
        mUploader->upload(writeFile("first.png"), "pomf");
        mUploader->upload(writeFile("second.png"), "pomf");
        mNetwork->replies.at(0)->sendProgress(80, 100);
        mNetwork->replies.at(1)->sendProgress(60, 100);
        QCOMPARE(mUploader->progress(), 70);

        mNetwork->replies.at(0)->finish(QByteArray(), QNetworkReply::HostNotFoundError);

        QCOMPARE(mUploader->uploading(), 1);
        QCOMPARE(mUploader->progress(), 0); // Current behaviour: one failure zeroes the total while the other upload runs.
    }

    void cancel()
    {
        const QString first = writeFile("first.png");
        const QString second = writeFile("second.png");
        QSignalSpy cancelSpy(mUploader, &Uploader::cancelAll);

        mUploader->upload(first, "pomf");
        mUploader->upload(second, "pomf");
        mNetwork->replies.at(0)->sendProgress(50, 100);
        QCOMPARE(mUploader->progress(), 25);

        mUploader->cancel();

        QCOMPARE(cancelSpy.count(), 1);
        QCOMPARE(mUploader->uploading(), 0);
        QCOMPARE(mUploader->progress(), 0);

        // Current behaviour: the pomf uploader aborts its reply, and an aborted reply looks like a network
        // failure (NetworkError, not CancelError), so each cancelled upload reports an error before done.
        QCOMPARE(mEvents, QStringList()
                 << "error:Error reaching uploader" << doneEvent(first)
                 << "error:Error reaching uploader" << doneEvent(second));
    }

    void cancelWithNothingRunning()
    {
        QSignalSpy cancelSpy(mUploader, &Uploader::cancelAll);
        QSignalSpy progressSpy(mUploader, &Uploader::progressChanged);

        mUploader->cancel();

        QCOMPARE(cancelSpy.count(), 1);
        QCOMPARE(progressSpy.count(), 0);
        QVERIFY(mEvents.isEmpty());
    }

    void reportProgressWithNothingRunning()
    {
        QSignalSpy progressSpy(mUploader, &Uploader::progressChanged);

        mUploader->reportProgress(42);

        QCOMPARE(mUploader->progress(), 42);
        QCOMPARE(progressSpy.count(), 1);
        QCOMPARE(progressSpy.last().at(0).toInt(), 42);
    }

    // S02 flow: a Screenshot no longer calls the uploader or the history itself. It emits uploadRequested, the
    // manager hands that to its uploader, and the uploader's done reaches the history through the manager.
    void screenshotUploadFlow()
    {
        mSettings->setValue("options/history", true);

        Screenshot::Options options;
        options.result = Screenshot::Failure;
        options.format = Screenshot::PNG;
        options.namingOptions = { Screenshot::Empty, false, 0, QString() };
        options.directory = QDir(mDir->path());
        options.prefix = QStringLiteral("flow");
        options.uploadService = QStringLiteral("pomf");
        options.mode = Screenshot::None;
        options.quality = 80;
        options.animations = options.clipboard = options.urlClipboard = options.currentMonitor = false;
        options.cursor = options.magnify = options.optimize = options.preview = false;
        options.replace = options.saveAs = false;
        options.file = true;
        options.upload = true;

        QScopedPointer<Screenshot> screenshot(new Screenshot(nullptr, options));
        mManager->connectScreenshot(screenshot.data());
        QSignalSpy savedSpy(screenshot.data(), &Screenshot::saved);
        QSignalSpy requestSpy(screenshot.data(), &Screenshot::uploadRequested);

        QPixmap pixmap(8, 8);
        pixmap.fill(Qt::red);
        screenshot->pixmap() = pixmap;
        screenshot->save();

        const QString file = QDir::toNativeSeparators(mDir->filePath("flow.png"));
        QCOMPARE(screenshot->options().result, Screenshot::Success);
        QCOMPARE(screenshot->options().fileName, file);
        QCOMPARE(savedSpy.count(), 0); // With upload on, the history row is written when the upload is done.
        QCOMPARE(requestSpy.count(), 1);
        QCOMPARE(requestSpy.at(0).at(0).toString(), file);
        QCOMPARE(requestSpy.at(0).at(1).toString(), QStringLiteral("pomf"));

        QCOMPARE(mUploader->uploading(), 1);
        QCOMPARE(mNetwork->replies.size(), 1);
        mNetwork->replies.first()->finish("{\"success\": true, \"files\": [{\"url\": \"https://pomf.example/flow.png\"}]}");

        QCOMPARE(mEvents, QStringList() << doneEvent(file, "https://pomf.example/flow.png"));

        {
            QSqlQuery query("SELECT fileName, URL FROM history");
            QVERIFY(query.next());
            QCOMPARE(query.value(0).toString(), file);
            QCOMPARE(query.value(1).toString(), QStringLiteral("https://pomf.example/flow.png"));
            QVERIFY(!query.next());
        }

        // Close the history so the temporary directory can be removed on Windows.
        QSqlDatabase::database(QSqlDatabase::defaultConnection, false).close();
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    }
};

QTEST_MAIN(tst_Uploader)
#include "tst_uploader.moc"
