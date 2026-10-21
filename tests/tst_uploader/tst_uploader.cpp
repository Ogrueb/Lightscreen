// Stage 4 safety net: the uploader core (tools/uploader) and the custom uploader that replaced pomf in S03.
// Records current behaviour, not intended behaviour: assertions marked "Current behaviour" document quirks
// a later step may change on purpose. Since S03 the custom uploader is the only service.
#include <QtTest>
#include <cstring>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QLabel>
#include <QPushButton>
#include <QTemporaryDir>

#include <tools/screenshot.h>
#include <tools/screenshotmanager.h>
#include <tools/uploader/imageuploader.h>
#include <tools/uploader/customuploader.h>
#include <tools/uploader/uploader.h>
#include <widgets/uploaderoptionswidget.h>

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

    // httpStatus: the HTTP status the reply reports; by default 200 without a network error and none with one.
    void finish(const QByteArray &body, NetworkError networkError = NoError, int httpStatus = -1)
    {
        mBody = body;
        mOffset = 0;

        if (networkError != NoError) {
            setError(networkError, QStringLiteral("canned error"));
        }

        if (httpStatus < 0 && networkError == NoError) {
            httpStatus = 200;
        }

        if (httpStatus > 0) {
            setAttribute(QNetworkRequest::HttpStatusCodeAttribute, httpStatus);
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
class CustomProbe : public CustomUploader
{
public:
    using CustomUploader::CustomUploader;

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
        mSettings->setValue("upload/custom/url", "https://up.example/upload.php");
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

        // S03: every index maps to the custom uploader, so an old Imgur (0) or pomf (1) setting both work.
        QTest::newRow("0") << 0 << "custom";
        QTest::newRow("1 (pomf before M12)") << 1 << "custom";
        QTest::newRow("2") << 2 << "custom";
        QTest::newRow("-1") << -1 << "custom";
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

        // Names the factory does not know reach the custom uploader instead of a null pointer (bug 21).
        QTest::newRow("custom") << "custom" << "CustomUploader";
        QTest::newRow("pomf (before S03)") << "pomf" << "CustomUploader";
        QTest::newRow("removed service") << "imgur" << "CustomUploader";
        QTest::newRow("case") << "Custom" << "CustomUploader";
        QTest::newRow("empty") << "" << "CustomUploader";
        QTest::newRow("unknown") << "unknown" << "CustomUploader";
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
        mSettings->setValue("upload/custom/kept", "untouched");
        mSettings->setValue("upload/custom/group/nested", "ignored");

        QVariantHash saved;
        saved["url"] = "https://other.example/up";
        saved["number"] = 42;
        ImageUploader::saveSettings(mSettings, "custom", saved);

        QCOMPARE(mSettings->value("upload/custom/url").toString(), QStringLiteral("https://other.example/up"));

        QVariantHash expected = saved;
        expected["kept"] = "untouched"; // saveSettings merges: keys missing from the hash are left in place.
        // loadSettings reads child keys only, so "group/nested" is not returned.
        QCOMPARE(ImageUploader::loadSettings(mSettings, "custom"), expected);

        QVERIFY(ImageUploader::loadSettings(mSettings, "nothing-here").isEmpty());

        // Current behaviour: once written to the INI file and read back, numbers come back as strings.
        mSettings->sync();
        QSettings reopened(mSettings->fileName(), QSettings::IniFormat);
        const QVariantHash reloaded = ImageUploader::loadSettings(&reopened, "custom");

        QCOMPARE(reloaded["number"], QVariant(QStringLiteral("42")));
        QCOMPARE(reloaded["url"], QVariant(QStringLiteral("https://other.example/up")));
    }

    void instanceSettingsRoundTrip()
    {
        CustomProbe probe(mSettings, mNetwork); // The constructor loads "upload/custom".
        QCOMPARE(probe.settingsHash().value("url").toString(), QStringLiteral("https://up.example/upload.php"));

        probe.settingsHash()["url"] = "https://changed.example";
        probe.saveSettings();
        QCOMPARE(mSettings->value("upload/custom/url").toString(), QStringLiteral("https://changed.example"));

        mSettings->setValue("upload/custom/url", "https://external.example");
        probe.loadSettings();
        QCOMPARE(probe.settingsHash().value("url").toString(), QStringLiteral("https://external.example"));

        QVariantHash unsaved;
        unsaved["url"] = "https://unsaved.example";
        probe.setUploaderSettings(unsaved);
        QCOMPARE(probe.settingsHash(), unsaved);
        QCOMPARE(mSettings->value("upload/custom/url").toString(), QStringLiteral("https://external.example"));
    }

    void uploadEmptyFileNameDoesNothing()
    {
        mUploader->upload(QString(), "custom");

        QCOMPARE(mUploader->uploading(), 0);
        QVERIFY(mNetwork->replies.isEmpty());
        QVERIFY(mEvents.isEmpty());
    }

    // The defaults: field "files[]", no extra headers, link at "files.0.url" (what pomf hosts answer).
    void uploadDone()
    {
        const QString file = writeFile("shot.png");
        QSignalSpy progressSpy(mUploader, &Uploader::progressChanged);

        mUploader->upload(file, "custom");

        QCOMPARE(mUploader->uploading(), 1);
        QCOMPARE(mNetwork->replies.size(), 1);
        CannedReply *reply = mNetwork->replies.first();
        QCOMPARE(reply->operation(), QNetworkAccessManager::PostOperation);
        QCOMPARE(reply->url(), QUrl("https://up.example/upload.php"));
        QVERIFY(reply->request().header(QNetworkRequest::ContentTypeHeader).toString().startsWith("multipart/form-data; boundary="));
        // Header names are compared case-insensitively: Qt 6.7 and later send them in lower case.
        const QByteArray body = reply->sentBody.toLower();
        QVERIFY2(body.contains("content-disposition: form-data; name=\"files[]\"; filename=\"shot.png\""), reply->sentBody.constData());
        QVERIFY2(body.contains("content-type: image/png"), reply->sentBody.constData());
        QVERIFY(reply->sentBody.contains("not really an image"));

        reply->finish("{\"success\": true, \"files\": [{\"url\": \"https://up.example/abc.png\"}]}");

        QCOMPARE(mEvents, QStringList() << doneEvent(file, "https://up.example/abc.png"));
        QCOMPARE(mUploader->lastUrl(), QStringLiteral("https://up.example/abc.png"));
        QCOMPARE(mUploader->uploading(), 0);
        QCOMPARE(mUploader->progress(), 0);
        QCOMPARE(progressSpy.count(), 0);
    }

    void requestFieldAndHeaders()
    {
        mSettings->setValue("upload/custom/url", "up.example/api/1/upload"); // No scheme: read as http.
        mSettings->setValue("upload/custom/field", "image");
        mSettings->setValue("upload/custom/headers",
                            "Authorization: Bearer abc123\n"
                            "\n"
                            "not a header\n"
                            ": no name\n"
                            "  X-Spaced  :   padded value  \r\n"
                            "X-Colon: a: b");

        const QString file = writeFile("shot.jpg");
        mUploader->upload(file, "custom");

        QCOMPARE(mNetwork->replies.size(), 1);
        const QNetworkRequest request = mNetwork->replies.first()->request();
        QCOMPARE(request.url(), QUrl("http://up.example/api/1/upload"));
        QCOMPARE(request.rawHeader("Authorization"), QByteArray("Bearer abc123"));
        QCOMPARE(request.rawHeader("X-Spaced"), QByteArray("padded value"));
        QCOMPARE(request.rawHeader("X-Colon"), QByteArray("a: b"));
        QVERIFY(!request.hasRawHeader("not a header"));

        // The headers set by the form plus Content-Type, Content-Length or MIME-Version that Qt adds itself.
        QStringList custom;
        for (const QByteArray &name : request.rawHeaderList()) {
            if (name.startsWith("X-") || name == "Authorization") {
                custom << QString::fromLatin1(name);
            }
        }
        custom.sort();
        QCOMPARE(custom.join(','), QStringLiteral("Authorization,X-Colon,X-Spaced"));

        const QByteArray body = mNetwork->replies.first()->sentBody.toLower();
        QVERIFY2(body.contains("content-disposition: form-data; name=\"image\"; filename=\"shot.jpg\""), body.constData());
        QVERIFY2(body.contains("content-type: image/jpeg"), body.constData());
    }

    void customResponse_data()
    {
        QTest::addColumn<QString>("linkPath");
        QTest::addColumn<QString>("deletePath");
        QTest::addColumn<QByteArray>("body");
        QTest::addColumn<int>("networkError");
        QTest::addColumn<int>("httpStatus"); // -1: 200 without a network error, none with one
        QTest::addColumn<QStringList>("events"); // with "%f" standing for the file name

        const int noError = QNetworkReply::NoError;
        const QString defaultPath; // empty: the uploader uses "files.0.url"

        QJsonParseError parseError;
        QJsonDocument::fromJson("<html>502 Bad Gateway</html>", &parseError);
        const QString notJson = "error:Invalid response from uploader: " + parseError.errorString();

        QTest::newRow("pomf success, first file")
                << defaultPath << QString()
                << QByteArray("{\"success\": true, \"files\": [{\"url\": \"https://x/1.png\"}, {\"url\": \"https://x/2.png\"}]}")
                << noError << -1 << QStringList{ "done:%f|https://x/1.png|" };
        QTest::newRow("link and delete paths")
                << "data.link" << "data.delete"
                << QByteArray("{\"data\": {\"link\": \"https://i/x.png\", \"delete\": \"https://i/del/x\"}}")
                << noError << -1 << QStringList{ "done:%f|https://i/x.png|https://i/del/x" };
        QTest::newRow("delete path that does not resolve")
                << "data.link" << "data.nothing"
                << QByteArray("{\"data\": {\"link\": \"https://i/x.png\"}}")
                << noError << -1 << QStringList{ "done:%f|https://i/x.png|" };
        QTest::newRow("top-level array")
                << "0.url" << QString()
                << QByteArray("[{\"url\": \"https://a/1.png\"}]")
                << noError << -1 << QStringList{ "done:%f|https://a/1.png|" };
        QTest::newRow("HTTP 201")
                << "url" << QString()
                << QByteArray("{\"url\": \"https://a/1.png\"}")
                << noError << 201 << QStringList{ "done:%f|https://a/1.png|" };
        QTest::newRow("empty files array")
                << defaultPath << QString()
                << QByteArray("{\"success\": true, \"files\": []}")
                << noError << -1 << QStringList{ "error:No link in the response: \"files.0\" not found", "done:%f||" };
        QTest::newRow("empty link")
                << defaultPath << QString()
                << QByteArray("{\"files\": [{\"url\": \"\"}]}")
                << noError << -1 << QStringList{ "error:No link in the response: \"files.0.url\" is empty", "done:%f||" };
        QTest::newRow("link is not a string")
                << "id" << QString()
                << QByteArray("{\"id\": 42}")
                << noError << -1 << QStringList{ "error:No link in the response: \"id\" is not a string", "done:%f||" };
        // A pomf host's failure answer has no link; its "description" is not read any more.
        QTest::newRow("pomf failure")
                << defaultPath << QString()
                << QByteArray("{\"success\": false, \"description\": \"File too big\"}")
                << noError << -1 << QStringList{ "error:No link in the response: \"files\" not found", "done:%f||" };
        QTest::newRow("not JSON")
                << defaultPath << QString()
                << QByteArray("<html>502 Bad Gateway</html>")
                << noError << -1 << QStringList{ notJson, "done:%f||" };
        QTest::newRow("HTTP 500 with a link in the body")
                << defaultPath << QString()
                << QByteArray("{\"success\": true, \"files\": [{\"url\": \"https://x/1.png\"}]}")
                << int(QNetworkReply::InternalServerError) << 500 << QStringList{ "error:Upload failed: HTTP 500", "done:%f||" };
        QTest::newRow("HTTP 404, empty body")
                << defaultPath << QString()
                << QByteArray()
                << int(QNetworkReply::ContentNotFoundError) << 404 << QStringList{ "error:Upload failed: HTTP 404", "done:%f||" };
        QTest::newRow("network error, no HTTP status")
                << defaultPath << QString()
                << QByteArray()
                << int(QNetworkReply::HostNotFoundError) << -1 << QStringList{ "error:Error reaching uploader: canned error", "done:%f||" };
    }

    void customResponse()
    {
        QFETCH(QString, linkPath);
        QFETCH(QString, deletePath);
        QFETCH(QByteArray, body);
        QFETCH(int, networkError);
        QFETCH(int, httpStatus);
        QFETCH(QStringList, events);

        if (!linkPath.isEmpty()) {
            mSettings->setValue("upload/custom/linkPath", linkPath);
        }

        mSettings->setValue("upload/custom/deletePath", deletePath);

        const QString file = writeFile("shot.png");
        mUploader->upload(file, "custom");
        QCOMPARE(mNetwork->replies.size(), 1);

        mNetwork->replies.first()->finish(body, QNetworkReply::NetworkError(networkError), httpStatus);

        events.replaceInStrings("%f", file);
        QCOMPARE(mEvents, events);
        QCOMPARE(mUploader->uploading(), 0);
    }

    void resolvePath_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<QString>("path");
        QTest::addColumn<QString>("result");
        QTest::addColumn<QString>("errorString");

        const QByteArray nested("{\"files\": [{\"url\": \"u0\"}, {\"url\": \"u1\", \"n\": 3}], "
                                "\"data\": {\"links\": [[\"a\", \"b\"]], \"0\": \"zero\", \"empty\": \"\"}}");

        QTest::newRow("object member") << QByteArray("{\"link\": \"L\"}") << "link" << "L" << QString();
        QTest::newRow("default path") << nested << "files.0.url" << "u0" << QString();
        QTest::newRow("second element") << nested << "files.1.url" << "u1" << QString();
        QTest::newRow("array in array") << nested << "data.links.0.1" << "b" << QString();
        QTest::newRow("number as an object key") << nested << "data.0" << "zero" << QString();
        QTest::newRow("spaces around the path") << nested << " files.0.url " << "u0" << QString();
        QTest::newRow("missing key") << nested << "files.0.link" << QString() << "\"files.0.link\" not found";
        QTest::newRow("index out of range") << nested << "files.2.url" << QString() << "\"files.2\" not found";
        QTest::newRow("negative index") << nested << "files.-1.url" << QString() << "\"files.-1\" not found";
        QTest::newRow("name on an array") << nested << "files.url" << QString() << "\"files.url\" not found";
        QTest::newRow("past a string") << nested << "files.0.url.x" << QString() << "\"files.0.url.x\" not found";
        QTest::newRow("number") << nested << "files.1.n" << QString() << "\"files.1.n\" is not a string";
        QTest::newRow("object") << nested << "data" << QString() << "\"data\" is not a string";
        QTest::newRow("empty string") << nested << "data.empty" << QString() << "\"data.empty\" is empty";
        QTest::newRow("empty path") << nested << "" << QString() << "no path given";
    }

    void resolvePath()
    {
        QFETCH(QByteArray, json);
        QFETCH(QString, path);
        QFETCH(QString, result);
        QFETCH(QString, errorString);

        const QJsonDocument document = QJsonDocument::fromJson(json);
        QVERIFY(!document.isNull());

        QString error;
        QCOMPARE(CustomUploader::resolvePath(QJsonValue(document.object()), path, &error), result);
        QCOMPARE(error, errorString);
        QCOMPARE(CustomUploader::resolvePath(QJsonValue(document.object()), path).isNull(), result.isNull());
    }

    // The custom uploader fails inside upload(), before any request is made.
    void errorBeforeRequest_data()
    {
        QTest::addColumn<QString>("url");
        QTest::addColumn<bool>("fileExists");
        QTest::addColumn<QString>("errorString");

        QTest::newRow("no URL") << QString() << true << "No upload URL set";
        QTest::newRow("only spaces") << "   " << true << "No upload URL set";
        QTest::newRow("not HTTP") << "ftp://up.example/upload" << true << "Invalid upload URL: ftp://up.example/upload";
        QTest::newRow("unreadable file") << "https://up.example/upload.php" << false << "Unable to read screenshot file";
    }

    void errorBeforeRequest()
    {
        QFETCH(QString, url);
        QFETCH(bool, fileExists);
        QFETCH(QString, errorString);

        mSettings->setValue("upload/custom/url", url);
        const QString file = fileExists ? writeFile("shot.png") : mDir->filePath("missing.png");

        mUploader->upload(file, "custom");

        QVERIFY(mNetwork->replies.isEmpty());
        QCOMPARE(mEvents, QStringList() << "error:" + errorString << doneEvent(file));
        QCOMPARE(mUploader->uploading(), 0);
    }

    // Bug 21 (fixed in M12): an unknown service name used to reach a null uploader and crash. It now goes to the
    // custom uploader, which with no URL set reports an error and finishes the upload.
    void uploadUnknownService()
    {
        mSettings->setValue("upload/custom/url", QString());
        const QString file = writeFile("shot.png");

        mUploader->upload(file, "unknown");

        QVERIFY(mNetwork->replies.isEmpty());
        QCOMPARE(mEvents, QStringList() << "error:No upload URL set" << doneEvent(file));
        QCOMPARE(mUploader->uploading(), 0);
    }

    void migration_data()
    {
        QTest::addColumn<QVariant>("pomfUrl");   // invalid: key absent
        QTest::addColumn<QVariant>("customUrl"); // invalid: key absent
        QTest::addColumn<bool>("migrated");
        QTest::addColumn<QString>("url");        // upload/custom/url afterwards; null: key absent

        QTest::newRow("pomf URL, no custom URL") << QVariant("https://pomf.example") << QVariant() << true << "https://pomf.example/upload.php";
        QTest::newRow("trailing slash") << QVariant("https://pomf.example/") << QVariant() << true << "https://pomf.example/upload.php";
        QTest::newRow("no scheme") << QVariant("pomf.example") << QVariant() << true << "pomf.example/upload.php";
        QTest::newRow("custom URL already set") << QVariant("https://pomf.example") << QVariant("https://mine.example/up") << false << "https://mine.example/up";
        QTest::newRow("custom URL saved empty") << QVariant("https://pomf.example") << QVariant("") << false << "";
        QTest::newRow("pomf URL empty") << QVariant("") << QVariant() << false << QString();
        QTest::newRow("no pomf URL") << QVariant() << QVariant() << false << QString();
    }

    void migration()
    {
        QFETCH(QVariant, pomfUrl);
        QFETCH(QVariant, customUrl);
        QFETCH(bool, migrated);
        QFETCH(QString, url);

        QSettings settings(mDir->filePath("migration.ini"), QSettings::IniFormat);

        if (pomfUrl.isValid()) {
            settings.setValue("upload/pomf/pomf_url", pomfUrl);
        }

        if (customUrl.isValid()) {
            settings.setValue("upload/custom/url", customUrl);
        }

        QCOMPARE(CustomUploader::migrateSettings(&settings), migrated);
        QCOMPARE(CustomUploader::migrateSettings(&settings), false); // Once only.

        QCOMPARE(settings.contains("upload/custom/url"), !url.isNull());
        QCOMPARE(settings.value("upload/custom/url").toString(), url);
        QCOMPARE(settings.value("upload/pomf/pomf_url"), pomfUrl.isValid() ? QVariant(pomfUrl.toString()) : QVariant()); // Left in place.

        if (migrated) {
            QCOMPARE(settings.value("upload/custom/field").toString(), QStringLiteral("files[]"));
            QCOMPARE(settings.value("upload/custom/linkPath").toString(), QStringLiteral("files.0.url"));
        } else {
            QVERIFY(!settings.contains("upload/custom/field"));
            QVERIFY(!settings.contains("upload/custom/linkPath"));
        }
    }

    // Settings from before S03 with only a pomf URL upload to the same host, the way the pomf uploader did.
    void migratedPomfSettingsUpload()
    {
        mSettings->remove("upload/custom");
        mSettings->setValue("upload/pomf/pomf_url", "https://pomf.example");

        const QString file = writeFile("shot.png");
        mUploader->upload(file, "custom");

        QCOMPARE(mNetwork->replies.size(), 1);
        QCOMPARE(mNetwork->replies.first()->url(), QUrl("https://pomf.example/upload.php"));
        QVERIFY(mNetwork->replies.first()->sentBody.toLower().contains("name=\"files[]\""));

        mNetwork->replies.first()->finish("{\"success\": true, \"files\": [{\"url\": \"https://pomf.example/abc.png\"}]}");
        QCOMPARE(mEvents, QStringList() << doneEvent(file, "https://pomf.example/abc.png"));
        QCOMPARE(mSettings->value("upload/custom/url").toString(), QStringLiteral("https://pomf.example/upload.php"));
    }

    // The options widget's "Test upload": posts a 1x1 PNG with the values in the form (not the saved ones) and shows
    // the link or the error. It never goes through the app's Uploader, so no upload is reported there.
    void optionsWidgetTestUpload()
    {
        UploaderOptionsWidget widget;
        auto button = widget.findChild<QPushButton *>("testButton");
        auto cancelButton = widget.findChild<QPushButton *>("cancelButton");
        auto result = widget.findChild<QLabel *>("resultLabel");
        QVERIFY(button && cancelButton && result);

        QVariantHash form;
        form["url"] = "https://form.example/upload";
        form["field"] = "image";
        form["headers"] = "X-Key: secret";
        form["linkPath"] = "data.link";
        form["deletePath"] = "";
        widget.setUploaderSettings(form);
        QCOMPARE(widget.uploaderSettings(), form);
        QVERIFY(button->isEnabled());

        // Success.
        button->click();
        QCOMPARE(mNetwork->replies.size(), 1);
        CannedReply *reply = mNetwork->replies.last();
        QCOMPARE(reply->url(), QUrl("https://form.example/upload"));
        QCOMPARE(reply->request().rawHeader("X-Key"), QByteArray("secret"));
        QVERIFY(reply->sentBody.toLower().contains("name=\"image\""));
        QVERIFY(reply->sentBody.contains("\x89PNG"));
        QVERIFY(!button->isEnabled());

        reply->finish("{\"data\": {\"link\": \"https://form.example/t.png\"}}");
        QVERIFY(button->isEnabled());
        QVERIFY2(result->text().contains("https://form.example/t.png"), qPrintable(result->text()));

        // An error.
        button->click();
        QCOMPARE(mNetwork->replies.size(), 2);
        mNetwork->replies.last()->finish(QByteArray(), QNetworkReply::InternalServerError, 500);
        QCOMPARE(result->text(), QStringLiteral("Upload failed: HTTP 500"));

        // Cancelled.
        button->click();
        QCOMPARE(mNetwork->replies.size(), 3);
        cancelButton->click();
        QCOMPARE(result->text(), QStringLiteral("Cancelled"));
        QVERIFY(button->isEnabled());

        // No URL: the button is disabled.
        form["url"] = "";
        widget.setUploaderSettings(form);
        QVERIFY(!button->isEnabled());

        // Defaults shown for a fresh setting.
        widget.setUploaderSettings(QVariantHash());
        QCOMPARE(widget.uploaderSettings().value("field").toString(), QStringLiteral("files[]"));
        QCOMPARE(widget.uploaderSettings().value("linkPath").toString(), QStringLiteral("files.0.url"));

        QVERIFY(mEvents.isEmpty());
        QCOMPARE(mSettings->value("upload/custom/url").toString(), QStringLiteral("https://up.example/upload.php"));
    }

    void progressOverTwoUploads()
    {
        const QString first = writeFile("first.png");
        const QString second = writeFile("second.png");
        QSignalSpy progressSpy(mUploader, &Uploader::progressChanged);

        mUploader->upload(first, "custom");
        mUploader->upload(second, "custom");
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
        mUploader->upload(writeFile("first.png"), "custom");
        mUploader->upload(writeFile("second.png"), "custom");
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

        mUploader->upload(first, "custom");
        mUploader->upload(second, "custom");
        mNetwork->replies.at(0)->sendProgress(50, 100);
        QCOMPARE(mUploader->progress(), 25);

        mUploader->cancel();

        QCOMPARE(cancelSpy.count(), 1);
        QCOMPARE(mUploader->uploading(), 0);
        QCOMPARE(mUploader->progress(), 0);

        // Bug 22 (fixed in S03): the uploader aborts its reply and reports the aborted reply as CancelError, which
        // Uploader does not pass on, so each cancelled upload ends with done and no error.
        QCOMPARE(mEvents, QStringList() << doneEvent(first) << doneEvent(second));
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
        options.uploadService = QStringLiteral("custom");
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
        QCOMPARE(requestSpy.at(0).at(1).toString(), QStringLiteral("custom"));

        QCOMPARE(mUploader->uploading(), 1);
        QCOMPARE(mNetwork->replies.size(), 1);
        mNetwork->replies.first()->finish("{\"success\": true, \"files\": [{\"url\": \"https://up.example/flow.png\"}]}");

        QCOMPARE(mEvents, QStringList() << doneEvent(file, "https://up.example/flow.png"));

        {
            QSqlQuery query("SELECT fileName, URL FROM history");
            QVERIFY(query.next());
            QCOMPARE(query.value(0).toString(), file);
            QCOMPARE(query.value(1).toString(), QStringLiteral("https://up.example/flow.png"));
            QVERIFY(!query.next());
        }

        // Close the history so the temporary directory can be removed on Windows.
        QSqlDatabase::database(QSqlDatabase::defaultConnection, false).close();
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    }
};

QTEST_MAIN(tst_Uploader)
#include "tst_uploader.moc"
