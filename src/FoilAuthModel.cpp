/*
 * Copyright (C) 2019-2020 Jolla Ltd.
 * Copyright (C) 2019-2020 Slava Monich <slava@monich.com>
 *
 * You may use this file under the terms of BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *   3. Neither the names of the copyright holders nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "FoilAuthModel.h"
#include "FoilAuthToken.h"
#include "FoilAuth.h"

#include "HarbourDebug.h"
#include "HarbourTask.h"

#include "foil_output.h"
#include "foil_private_key.h"
#include "foil_random.h"
#include "foil_util.h"

#include "gutil_misc.h"

#include "foilmsg.h"

#include <unistd.h>
#include <sys/stat.h>

#define ENCRYPT_KEY_TYPE FOILMSG_KEY_AES_256
#define DIGEST_TYPE FOIL_DIGEST_MD5

#define HEADER_LABEL            "OTP-Label"
#define HEADER_ISSUER           "OTP-Issuer"
#define HEADER_DIGITS           "OTP-Digits"
#define HEADER_TIMESHIFT        "OTP-TimeShift"
#define HEADER_FAVORITE         "OTP-Favorite"

// Directories relative to home
#define FOIL_AUTH_DIR           "Documents/FoilAuth"
#define FOIL_KEY_DIR            ".local/share/foil"
#define FOIL_KEY_FILE           "foil.key"

#define INFO_FILE               ".info"
#define INFO_CONTENTS           "FoilAuth"
#define INFO_ORDER_HEADER       "Order"
#define INFO_ORDER_DELIMITER    ','
#define INFO_ORDER_DELIMITER_S  ","

// Model roles
#define FOILAUTH_ROLES_(first,role,last) \
    first(ModelId,modelId) \
    role(Favorite,favorite) \
    role(Secret,secret) \
    role(Issuer,issuer) \
    role(Digits,digits) \
    role(TimeShift,timeShift) \
    role(Label,label) \
    role(PrevPassword,prevPassword) \
    role(CurrentPassword,currentPassword) \
    last(NextPassword,nextPassword)

#define FOILAUTH_ROLES(role) \
    FOILAUTH_ROLES_(role,role,role)

// ==========================================================================
// FoilAuthModel::ModelData
// ==========================================================================

class FoilAuthModel::ModelData {
public:
    enum Role {
#define FIRST(X,x) FirstRole = Qt::UserRole, X##Role = FirstRole,
#define ROLE(X,x) X##Role,
#define LAST(X,x) X##Role, LastRole = X##Role
        FOILAUTH_ROLES_(FIRST,ROLE,LAST)
#undef FIRST
#undef ROLE
#undef LAST
    };

#define ROLE(X,x) static const QString RoleName##X;
    FOILAUTH_ROLES(ROLE)
#undef ROLE

    typedef QList<ModelData*> List;

    ModelData(QString aPath, QByteArray aSecret, QString aLabel, QString aIssuer,
        int aDigits = FoilAuthToken::DEFAULT_DIGITS,
        int aTimeShift = FoilAuthToken::DEFAULT_TIMESHIFT,
        bool aFavorite = true);
    ModelData(QString aPath, const FoilAuthToken& aToken, bool aFavorite = true);

    QVariant get(Role aRole) const;
    QString label() { return iToken.iLabel; }
    void setTokenPath(QString aPath);

    static QString headerString(const FoilMsg* aMsg, const char* aKey);
    static int headerInt(const FoilMsg* aMsg, const char* aKey, int aDefault);
    static bool headerBool(const FoilMsg* aMsg, const char* aKey, bool aDefault);

public:
    QString iPath;
    QString iId;
    bool iFavorite;
    FoilAuthToken iToken;
    QString iSecretBase32;
    QString iPrevPassword;
    QString iCurrentPassword;
    QString iNextPassword;
};

#define ROLE(X,x) const QString FoilAuthModel::ModelData::RoleName##X(#x);
FOILAUTH_ROLES(ROLE)
#undef ROLE

FoilAuthModel::ModelData::ModelData(QString aPath, QByteArray aSecret,
    QString aLabel, QString aIssuer, int aDigits, int aTimeShift,
    bool aFavorite) :
    iPath(aPath),
    iId(QFileInfo(aPath).fileName()),
    iFavorite(aFavorite),
    iToken(aSecret, aLabel, aIssuer, aDigits, aTimeShift),
    iSecretBase32(FoilAuth::toBase32(aSecret))
{
    HDEBUG(iSecretBase32 << aLabel);
}

FoilAuthModel::ModelData::ModelData(QString aPath, const FoilAuthToken& aToken,
    bool aFavorite) :
    iPath(aPath),
    iId(QFileInfo(aPath).fileName()),
    iFavorite(aFavorite),
    iToken(aToken),
    iSecretBase32(FoilAuth::toBase32(iToken.iBytes))
{
    HDEBUG(iSecretBase32 << iToken.iLabel);
}

void FoilAuthModel::ModelData::setTokenPath(QString aPath)
{
    iPath = aPath;
    iId = QFileInfo(aPath).fileName();
}

QVariant FoilAuthModel::ModelData::get(Role aRole) const
{
    switch (aRole) {
    case ModelIdRole: return iId;
    case FavoriteRole: return iFavorite;
    case SecretRole: return iSecretBase32;
    case IssuerRole: return iToken.iIssuer;
    case DigitsRole: return iToken.iDigits;
    case TimeShiftRole: return iToken.iTimeShift;
    case LabelRole: return iToken.iLabel;
    case PrevPasswordRole: return iPrevPassword;
    case CurrentPasswordRole: return iCurrentPassword;
    case NextPasswordRole: return iNextPassword;
    }
    return QVariant();
}

QString FoilAuthModel::ModelData::headerString(const FoilMsg* aMsg, const char* aKey)
{
    const char* value = foilmsg_get_value(aMsg, aKey);
    return (value && value[0]) ? QString::fromLatin1(value) : QString();
}

int FoilAuthModel::ModelData::headerInt(const FoilMsg* aMsg, const char* aKey, int aDefault)
{
    const char* str = foilmsg_get_value(aMsg, aKey);
    int value = aDefault;
    gutil_parse_int(str, 10, &value);
    return value;
}

bool FoilAuthModel::ModelData::headerBool(const FoilMsg* aMsg, const char* aKey, bool aDefault)
{
    const char* str = foilmsg_get_value(aMsg, aKey);
    int value = 0;
    return gutil_parse_int(str, 10, &value) && value;
}

// ==========================================================================
// FoilNotesModel::ModelInfo
// ==========================================================================

class FoilAuthModel::ModelInfo {
public:
    ModelInfo() {}
    ModelInfo(const FoilMsg* msg);
    ModelInfo(const ModelInfo& aInfo);
    ModelInfo(const ModelData::List aData);

    static ModelInfo load(QString aDir, FoilPrivateKey* aPrivate,
        FoilKey* aPublic);

    void save(QString aDir, FoilPrivateKey* aPrivate, FoilKey* aPublic);
    ModelInfo& operator = (const ModelInfo& aInfo);

public:
    QStringList iOrder;
};

FoilAuthModel::ModelInfo::ModelInfo(const ModelInfo& aInfo) :
    iOrder(aInfo.iOrder)
{
}

FoilAuthModel::ModelInfo& FoilAuthModel::ModelInfo::operator=(const ModelInfo& aInfo)
{
    iOrder = aInfo.iOrder;
    return *this;
}

FoilAuthModel::ModelInfo::ModelInfo(const ModelData::List aData)
{
    const int n = aData.count();
    if (n > 0) {
        iOrder.reserve(n);
        for (int i = 0; i < n; i++) {
            const ModelData* data = aData.at(i);
            iOrder.append(QFileInfo(data->iPath).fileName());
        }
    }
}

FoilAuthModel::ModelInfo::ModelInfo(const FoilMsg* msg)
{
    const char* order = foilmsg_get_value(msg, INFO_ORDER_HEADER);
    if (order) {
        char** strv = g_strsplit(order, INFO_ORDER_DELIMITER_S, -1);
        for (char** ptr = strv; *ptr; ptr++) {
            iOrder.append(g_strstrip(*ptr));
        }
        g_strfreev(strv);
        HDEBUG(order);
    }
}

FoilAuthModel::ModelInfo FoilAuthModel::ModelInfo::load(QString aDir,
    FoilPrivateKey* aPrivate, FoilKey* aPublic)
{
    ModelInfo info;
    QString fullPath(aDir + "/" INFO_FILE);
    const QByteArray path(fullPath.toUtf8());
    const char* fname = path.constData();
    HDEBUG("Loading" << fname);
    FoilMsg* msg = foilmsg_decrypt_file(aPrivate, fname, NULL);
    if (msg) {
        if (foilmsg_verify(msg, aPublic)) {
            info = ModelInfo(msg);
        } else {
            HWARN("Could not verify" << fname);
        }
        foilmsg_free(msg);
    }
    return info;
}

void FoilAuthModel::ModelInfo::save(QString aDir, FoilPrivateKey* aPrivate,
    FoilKey* aPublic)
{
    QString fullPath(aDir + "/" INFO_FILE);
    const QByteArray path(fullPath.toUtf8());
    const char* fname = path.constData();
    FoilOutput* out = foil_output_file_new_open(fname);
    if (out) {
        QString buf;
        const int n = iOrder.count();
        for (int i = 0; i < n; i++) {
            if (!buf.isEmpty()) buf += QChar(INFO_ORDER_DELIMITER);
            buf += iOrder.at(i);
        }

        const QByteArray order(buf.toUtf8());

        HDEBUG("Saving" << fname);
        HDEBUG(INFO_ORDER_HEADER ":" << order.constData());

        FoilMsgHeaders headers;
        FoilMsgHeader header[1];
        headers.header = header;
        headers.count = 0;
        header[headers.count].name = INFO_ORDER_HEADER;
        header[headers.count].value = order.constData();
        headers.count++;

        FoilMsgEncryptOptions opt;
        memset(&opt, 0, sizeof(opt));
        opt.key_type = ENCRYPT_KEY_TYPE;

        FoilBytes data;
        foil_bytes_from_string(&data, INFO_CONTENTS);
        foilmsg_encrypt(out, &data, NULL, &headers, aPrivate, aPublic,
            &opt, NULL);
        foil_output_unref(out);
    } else {
        HWARN("Failed to open" << fname);
    }
}

// ==========================================================================
// FoilAuthModel::BaseTask
// ==========================================================================

class FoilAuthModel::BaseTask : public HarbourTask {
    Q_OBJECT

public:
    BaseTask(QThreadPool* aPool, FoilPrivateKey* aPrivateKey,
        FoilKey* aPublicKey);
    virtual ~BaseTask();

    FoilMsg* decryptAndVerify(QString aFileName) const;
    FoilMsg* decryptAndVerify(const char* aFileName) const;

    static bool removeFile(QString aPath);
    static QByteArray toByteArray(const FoilMsg* aMsg);

public:
    FoilPrivateKey* iPrivateKey;
    FoilKey* iPublicKey;
};

FoilAuthModel::BaseTask::BaseTask(QThreadPool* aPool,
    FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey) :
    HarbourTask(aPool),
    iPrivateKey(foil_private_key_ref(aPrivateKey)),
    iPublicKey(foil_key_ref(aPublicKey))
{
}

FoilAuthModel::BaseTask::~BaseTask()
{
    foil_private_key_unref(iPrivateKey);
    foil_key_unref(iPublicKey);
}

bool FoilAuthModel::BaseTask::removeFile(QString aPath)
{
    if (!aPath.isEmpty()) {
        if (QFile::remove(aPath)) {
            HDEBUG("Removed" << qPrintable(aPath));
            return true;
        }
        HWARN("Failed to delete" << qPrintable(aPath));
    }
    return false;
}

FoilMsg* FoilAuthModel::BaseTask::decryptAndVerify(QString aFileName) const
{
    if (!aFileName.isEmpty()) {
        const QByteArray fileNameBytes(aFileName.toUtf8());
        return decryptAndVerify(fileNameBytes.constData());
    } else{
        return NULL;
    }
}

FoilMsg* FoilAuthModel::BaseTask::decryptAndVerify(const char* aFileName) const
{
    if (aFileName) {
        HDEBUG("Decrypting" << aFileName);
        FoilMsg* msg = foilmsg_decrypt_file(iPrivateKey, aFileName, NULL);
        if (msg) {
#if HARBOUR_DEBUG
            for (uint i = 0; i < msg->headers.count; i++) {
                const FoilMsgHeader* header = msg->headers.header + i;
                HDEBUG(" " << header->name << ":" << header->value);
            }
#endif // HARBOUR_DEBUG
            if (foilmsg_verify(msg, iPublicKey)) {
                return msg;
            } else {
                HWARN("Could not verify" << aFileName);
            }
            foilmsg_free(msg);
        }
    }
    return NULL;
}

QByteArray FoilAuthModel::BaseTask::toByteArray(const FoilMsg* aMsg)
{
    if (aMsg) {
        const char* type = aMsg->content_type;
        if (!type || g_str_has_prefix(type, "text/")) {
            gsize size;
            const char* data = (char*)g_bytes_get_data(aMsg->data, &size);
            // Base32 comes in 40-bit (5-byte) chunks
            if (data && size && !(size % 5)) {
                return QByteArray(data, size);
            }
        } else {
            HWARN("Unexpected content type" << type);
        }
    }
    return QByteArray();
}

// ==========================================================================
// FoilAuthModel::GenerateKeyTask
// ==========================================================================

class FoilAuthModel::GenerateKeyTask : public BaseTask {
    Q_OBJECT

public:
    GenerateKeyTask(QThreadPool* aPool, QString aKeyFile, int aBits,
        QString aPassword);

    virtual void performTask();

public:
    const QString iKeyFile;
    const int iBits;
    const QString iPassword;
};

FoilAuthModel::GenerateKeyTask::GenerateKeyTask(QThreadPool* aPool,
    QString aKeyFile, int aBits, QString aPassword) :
    BaseTask(aPool, NULL, NULL),
    iKeyFile(aKeyFile),
    iBits(aBits),
    iPassword(aPassword)
{
}

void FoilAuthModel::GenerateKeyTask::performTask()
{
    HDEBUG("Generating key..." << iBits << "bits");
    FoilKey* key = foil_key_generate_new(FOIL_KEY_RSA_PRIVATE, iBits);
    if (key) {
        GError* error = NULL;
        const QByteArray path(iKeyFile.toUtf8());
        const QByteArray passphrase(iPassword.toUtf8());
        FoilOutput* out = foil_output_file_new_open(path.constData());
        FoilPrivateKey* pk = FOIL_PRIVATE_KEY(key);
        if (foil_private_key_encrypt(pk, out, FOIL_KEY_EXPORT_FORMAT_DEFAULT,
            passphrase.constData(),
            NULL, &error)) {
            iPrivateKey = pk;
            iPublicKey = foil_public_key_new_from_private(pk);
        } else {
            HWARN(error->message);
            g_error_free(error);
            foil_key_unref(key);
        }
        foil_output_unref(out);
    }
    HDEBUG("Done!");
}

// ==========================================================================
// FoilAuthModel::EncryptTask
// ==========================================================================

class FoilAuthModel::EncryptTask : public BaseTask {
    Q_OBJECT

public:
    EncryptTask(QThreadPool* aPool, const ModelData* aData,
        FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey,
        quint64 aTime, QString aDestDir);

    virtual void performTask();

public:
    const QString iId;
    const bool iFavorite;
    const FoilAuthToken iToken;
    const QString iDestDir;
    const QString iRemoveFile;
    const quint64 iTime;
    QString iNewFile;
    QString iPrevPassword;
    QString iCurrentPassword;
    QString iNextPassword;
};

FoilAuthModel::EncryptTask::EncryptTask(QThreadPool* aPool,
    const ModelData* aData, FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey, quint64 aTime, QString aDestDir) :
    BaseTask(aPool, aPrivateKey, aPublicKey),
    iId(aData->iId),
    iFavorite(aData->iFavorite),
    iToken(aData->iToken),
    iDestDir(aDestDir),
    iRemoveFile(aData->iPath),
    iTime(aTime)
{
    HDEBUG("Encrypting" << iToken.iLabel);
}

void FoilAuthModel::EncryptTask::performTask()
{
    GString* dest = g_string_sized_new(iDestDir.size() + 9);
    FoilOutput* out = FoilAuth::createFoilFile(iDestDir, dest);
    if (out) {
        FoilMsgEncryptOptions opt;
        memset(&opt, 0, sizeof(opt));
        opt.key_type = ENCRYPT_KEY_TYPE;

        FoilMsgHeaders headers;
        FoilMsgHeader header[5];

        headers.header = header;
        headers.count = 0;

        const QByteArray label(iToken.iLabel.toUtf8());
        header[headers.count].name = HEADER_LABEL;
        header[headers.count].value = label.constData();
        headers.count++;

        QByteArray issuer;
        if (!iToken.iIssuer.isEmpty()) {
            issuer = iToken.iIssuer.toUtf8();
            header[headers.count].name = HEADER_ISSUER;
            header[headers.count].value = issuer.constData();
            headers.count++;
        }

        if (iFavorite) {
            header[headers.count].name = HEADER_FAVORITE;
            header[headers.count].value = "1";
            headers.count++;
        }

        char digits[16];
        snprintf(digits, sizeof(digits), "%d", iToken.iDigits);
        header[headers.count].name = HEADER_DIGITS;
        header[headers.count].value = digits;
        headers.count++;

        char timeshift[16];
        if (iToken.iTimeShift != FoilAuthToken::DEFAULT_TIMESHIFT) {
            snprintf(timeshift, sizeof(timeshift), "%d", iToken.iTimeShift);
            header[headers.count].name = HEADER_TIMESHIFT;
            header[headers.count].value = timeshift;
            headers.count++;
        }

        HASSERT(headers.count <= G_N_ELEMENTS(header));
        HDEBUG("Writing" << FoilAuth::toBase32(iToken.iBytes));

        FoilBytes body;
        body.val = (guint8*)iToken.iBytes.constData();
        body.len = iToken.iBytes.length();

        if (foilmsg_encrypt(out, &body, NULL, &headers,
            iPrivateKey, iPublicKey, &opt, NULL)) {
            iNewFile = QString::fromLocal8Bit(dest->str, dest->len);
            removeFile(iRemoveFile);
            foil_output_unref(out);
        } else {
            foil_output_unref(out);
            unlink(dest->str);
        }

        if (iTime && !isCanceled()) {
            iCurrentPassword = iToken.passwordString(iTime);
            iPrevPassword = iToken.passwordString(iTime - FoilAuth::PERIOD);
            iNextPassword = iToken.passwordString(iTime + FoilAuth::PERIOD);
            HDEBUG(qPrintable(iPrevPassword) << qPrintable(iCurrentPassword) <<
                qPrintable(iNextPassword));
        }
    }
    g_string_free(dest, TRUE);
}

// ==========================================================================
// FoilAuthModel::PasswordTask
// ==========================================================================

class FoilAuthModel::PasswordTask : public HarbourTask {
    Q_OBJECT

public:
    PasswordTask(QThreadPool* aPool, const ModelData* aData, quint64 aTime);

    virtual void performTask();

public:
    const QString iId;
    const FoilAuthToken iToken;
    const quint64 iTime;
    QString iPrevPassword;
    QString iCurrentPassword;
    QString iNextPassword;
};

FoilAuthModel::PasswordTask::PasswordTask(QThreadPool* aPool,
    const ModelData* aData, quint64 aTime) :
    HarbourTask(aPool),
    iId(aData->iId),
    iToken(aData->iToken),
    iTime(aTime)
{
    HDEBUG("Updating" << iToken.iLabel);
}

void FoilAuthModel::PasswordTask::performTask()
{
    iCurrentPassword = iToken.passwordString(iTime);
    iPrevPassword = iToken.passwordString(iTime - FoilAuth::PERIOD);
    iNextPassword = iToken.passwordString(iTime + FoilAuth::PERIOD);
    HDEBUG(iToken.iLabel << qPrintable(iPrevPassword) <<
        qPrintable(iCurrentPassword) << qPrintable(iNextPassword));
}

// ==========================================================================
// FoilNotesModel::SaveInfoTask
// ==========================================================================

class FoilAuthModel::SaveInfoTask : public BaseTask {
    Q_OBJECT

public:
    SaveInfoTask(QThreadPool* aPool, ModelInfo aInfo, QString aDir,
        FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey);

    virtual void performTask();

public:
    ModelInfo iInfo;
    QString iFoilDir;
};

FoilAuthModel::SaveInfoTask::SaveInfoTask(QThreadPool* aPool, ModelInfo aInfo,
    QString aFoilDir, FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey) :
    BaseTask(aPool, aPrivateKey, aPublicKey),
    iInfo(aInfo),
    iFoilDir(aFoilDir)
{
}

void FoilAuthModel::SaveInfoTask::performTask()
{
    if (!isCanceled()) {
        if (iInfo.iOrder.isEmpty()) {
            removeFile(iFoilDir + "/" INFO_FILE);
        } else {
            iInfo.save(iFoilDir, iPrivateKey, iPublicKey);
        }
    }
}

// ==========================================================================
// FoilAuthModel::DecryptAllTask
// ==========================================================================

class FoilAuthModel::DecryptAllTask : public BaseTask {
    Q_OBJECT

public:
    // The purpose of this class is to make sure that ModelData doesn't
    // get lost in transit when we asynchronously post the results from
    // DecryptAllTask to FoilAuthModel.
    //
    // If the signal successfully reaches the slot, the receiver zeros
    // iModelData which stops ModelData from being deallocated by the
    // Progress destructor. If the signal never reaches the slot, then
    // ModelData is deallocated together with when the last reference
    // to Progress
    class Progress {
    public:
        typedef QSharedPointer<Progress> Ptr;

        Progress(ModelData* aModelData, DecryptAllTask* aTask) :
            iModelData(aModelData), iTask(aTask) {}
        ~Progress() { delete iModelData; }

    public:
        ModelData* iModelData;
        DecryptAllTask* iTask;
    };

    DecryptAllTask(QThreadPool* aPool, QString aDir,
        FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey);

    virtual void performTask();

    bool decryptToken(QString aPath);

Q_SIGNALS:
    void progress(DecryptAllTask::Progress::Ptr aProgress);

public:
    const QString iDir;
    bool iSaveInfo;
    quint64 iTaskTime;
};

Q_DECLARE_METATYPE(FoilAuthModel::DecryptAllTask::Progress::Ptr)

FoilAuthModel::DecryptAllTask::DecryptAllTask(QThreadPool* aPool,
    QString aDir, FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey) :
    BaseTask(aPool, aPrivateKey, aPublicKey),
    iDir(aDir),
    iSaveInfo(false),
    iTaskTime(0)
{
}

bool FoilAuthModel::DecryptAllTask::decryptToken(QString aPath)
{
    bool ok = false;
    FoilMsg* msg = decryptAndVerify(aPath);
    if (msg) {
        QByteArray bytes(FoilAuth::toByteArray(msg->data));
        if (bytes.length() > 0) {
            ModelData* data = new ModelData(aPath, bytes,
                ModelData::headerString(msg, HEADER_LABEL),
                ModelData::headerString(msg, HEADER_ISSUER),
                ModelData::headerInt(msg, HEADER_DIGITS, FoilAuthToken::DEFAULT_DIGITS),
                ModelData::headerInt(msg, HEADER_TIMESHIFT, FoilAuthToken::DEFAULT_TIMESHIFT),
                ModelData::headerBool(msg, HEADER_FAVORITE, false));

            // Calculate current passwords while we are on it
            data->iCurrentPassword = data->iToken.passwordString(iTaskTime);
            data->iPrevPassword = data->iToken.passwordString(iTaskTime - FoilAuth::PERIOD);
            data->iNextPassword = data->iToken.passwordString(iTaskTime + FoilAuth::PERIOD);

            HDEBUG("Loaded secret from" << qPrintable(aPath));
            // The Progress takes ownership of ModelData
            Q_EMIT progress(Progress::Ptr(new Progress(data, this)));
            ok = true;
        }
        foilmsg_free(msg);
    }
    return ok;
}

void FoilAuthModel::DecryptAllTask::performTask()
{
    if (!isCanceled()) {
        const QString path(iDir);
        HDEBUG("Checking" << iDir);

        // Record time
        iTaskTime = QDateTime::currentDateTime().toTime_t();

        QDir dir(path);
        QFileInfoList list = dir.entryInfoList(QDir::Files |
            QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);

        // Restore the order
        ModelInfo info(ModelInfo::load(iDir, iPrivateKey, iPublicKey));

        const QString infoFile(INFO_FILE);
        QHash<QString,QString> fileMap;
        int i;
        for (i = 0; i < list.count(); i++) {
            const QFileInfo& info = list.at(i);
            if (info.isFile()) {
                const QString name(info.fileName());
                if (name != infoFile) {
                    fileMap.insert(name, info.filePath());
                }
            }
        }

        // First decrypt files in known order
        for (i = 0; i < info.iOrder.count() && !isCanceled(); i++) {
            const QString name(info.iOrder.at(i));
            if (!fileMap.contains(name) ||
                !decryptToken(fileMap.take(name))) {
                // Broken order or something
                HDEBUG(qPrintable(name) << "oops!");
                iSaveInfo = true;
            }
        }

        // Followed by the remaining files in no particular order
        if (!fileMap.isEmpty()) {
            const QStringList remainingFiles = fileMap.values();
            HDEBUG("Remaining file(s)" << remainingFiles);
            for (i = 0; i < remainingFiles.count() && !isCanceled(); i++) {
                if (!decryptToken(remainingFiles.at(i))) {
                    HDEBUG(remainingFiles.at(i) << "was not expected");
                    iSaveInfo = true;
                }
            }
        }
    }
}

// ==========================================================================
// FoilAuthModel::Private
// ==========================================================================

// s(SignalName,signalName)
#define FOIL_QUEUED_SIGNALS(s) \
    s(Count,count) \
    s(Busy,busy) \
    s(KeyAvailable,keyAvailable) \
    s(TimerActive,timerActive) \
    s(FoilState,foilState) \
    s(TimeLeft,timeLeft)

class FoilAuthModel::Private : public QObject {
    Q_OBJECT

public:
    typedef void (FoilAuthModel::*SignalEmitter)();
    typedef uint SignalMask;

    enum Signal {
#define FOIL_SIGNAL_ENUM_(Name,name) Signal##Name##Changed,
        FOIL_QUEUED_SIGNALS(FOIL_SIGNAL_ENUM_)
#undef FOIL_SIGNAL_ENUM_
        SignalCount
    };

    Private(FoilAuthModel* aParent);
    ~Private();

public Q_SLOTS:
    void onDecryptAllProgress(DecryptAllTask::Progress::Ptr aProgress);
    void onDecryptAllTaskDone();
    void onEncryptTaskDone();
    void onPasswordTaskDone();
    void onSaveInfoDone();
    void onGenerateKeyTaskDone();
    void onTimer();

public:
    FoilAuthModel* parentModel();
    int rowCount() const;
    ModelData* dataAt(int aIndex) const;
    ModelData* findData(QString aId) const;
    int findDataPos(QString aId) const;
    void queueSignal(Signal aSignal);
    void emitQueuedSignals();
    void checkTimer();
    bool checkPassword(QString aPassword);
    bool changePassword(QString aOldPassword, QString aNewPassword);
    void setKeys(FoilPrivateKey* aPrivate, FoilKey* aPublic = NULL);
    void setFoilState(FoilState aState);
    void addToken(const FoilAuthToken& aToken, bool aFavorite = true);
    void insertModelData(ModelData* aData);
    void dataChanged(int aIndex, ModelData::Role aRole);
    void dataChanged(QList<int> aRows, ModelData::Role aRole);
    void destroyItemAt(int aIndex);
    bool destroyItemAndRemoveFilesAt(int aIndex);
    bool destroyLastItemAndRemoveFiles();
    void deleteToken(QString aId);
    void deleteAll();
    void clearModel();
    bool busy() const;
    void encrypt(const ModelData* aData);
    void updatePasswords(const ModelData* aData);
    void saveInfo();
    void generate(int aBits, QString aPassword);
    void lock(bool aTimeout);
    bool unlock(QString aPassword);
    bool encrypt(QString aBody, QColor aColor, uint aPageNr, uint aReqId);

public:
    SignalMask iQueuedSignals;
    Signal iFirstQueuedSignal;
    ModelData::List iData;
    FoilState iFoilState;
    QString iFoilDataDir;
    QString iFoilKeyDir;
    QString iFoilKeyFile;
    FoilPrivateKey* iPrivateKey;
    FoilKey* iPublicKey;
    QThreadPool* iThreadPool;
    SaveInfoTask* iSaveInfoTask;
    GenerateKeyTask* iGenerateKeyTask;
    DecryptAllTask* iDecryptAllTask;
    QList<EncryptTask*> iEncryptTasks;
    QList<PasswordTask*> iPasswordTasks;
    QTimer* iTimer;
    quint64 iLastTime;
    quint64 iLastPeriod;
    uint iTimeLeft;
};

FoilAuthModel::Private::Private(FoilAuthModel* aParent) :
    QObject(aParent),
    iQueuedSignals(0),
    iFirstQueuedSignal(SignalCount),
    iFoilState(FoilKeyMissing),
    iFoilDataDir(QDir::homePath() + "/" FOIL_AUTH_DIR),
    iFoilKeyDir(QDir::homePath() + "/" FOIL_KEY_DIR),
    iFoilKeyFile(iFoilKeyDir + "/" + FOIL_KEY_FILE),
    iPrivateKey(NULL),
    iPublicKey(NULL),
    iThreadPool(new QThreadPool(this)),
    iSaveInfoTask(NULL),
    iGenerateKeyTask(NULL),
    iDecryptAllTask(NULL),
    iTimer(new QTimer(this)),
    iLastTime(QDateTime::currentDateTime().toTime_t()),
    iLastPeriod(iLastTime/FoilAuth::PERIOD),
    iTimeLeft(FoilAuth::PERIOD - (iLastTime % FoilAuth::PERIOD))
{
    // Serialize the tasks:
    iThreadPool->setMaxThreadCount(1);
    qRegisterMetaType<DecryptAllTask::Progress::Ptr>("DecryptAllTask::Progress::Ptr");

    HDEBUG("Key file" << qPrintable(iFoilKeyFile));
    HDEBUG("Data dir" << qPrintable(iFoilDataDir));

    // Create the directories if necessary
    if (QDir().mkpath(iFoilKeyDir)) {
        const QByteArray dir(iFoilKeyDir.toUtf8());
        chmod(dir.constData(), 0700);
    }

    if (QDir().mkpath(iFoilDataDir)) {
        const QByteArray dir(iFoilDataDir.toUtf8());
        chmod(dir.constData(), 0700);
    }

    // Initialize the key state
    GError* error = NULL;
    const QByteArray path(iFoilKeyFile.toUtf8());
    FoilPrivateKey* key = foil_private_key_decrypt_from_file
        (FOIL_KEY_RSA_PRIVATE, path.constData(), NULL, &error);
    if (key) {
        HDEBUG("Key not encrypted");
        iFoilState = FoilKeyNotEncrypted;
        foil_private_key_unref(key);
    } else {
        if (error->domain == FOIL_ERROR) {
            if (error->code == FOIL_ERROR_KEY_ENCRYPTED) {
                HDEBUG("Key encrypted");
                iFoilState = FoilLocked;
            } else {
                HDEBUG("Key invalid" << error->code);
                iFoilState = FoilKeyInvalid;
            }
        } else {
            HDEBUG(error->message);
            iFoilState = FoilKeyMissing;
        }
        g_error_free(error);
    }

    iTimer->setSingleShot(true);
    connect(iTimer, SIGNAL(timeout()), SLOT(onTimer()));
}

FoilAuthModel::Private::~Private()
{
    foil_private_key_unref(iPrivateKey);
    foil_key_unref(iPublicKey);
    if (iSaveInfoTask) iSaveInfoTask->release();
    if (iGenerateKeyTask) iGenerateKeyTask->release();
    if (iDecryptAllTask) iDecryptAllTask->release();
    int i;
    for (i = 0; i < iEncryptTasks.count(); i++) {
        iEncryptTasks.at(i)->release();
    }
    iEncryptTasks.clear();
    for (i = 0; i < iPasswordTasks.count(); i++) {
        iPasswordTasks.at(i)->release();
    }
    iPasswordTasks.clear();
    iThreadPool->waitForDone();
    qDeleteAll(iData);
}

inline FoilAuthModel* FoilAuthModel::Private::parentModel()
{
    return qobject_cast<FoilAuthModel*>(parent());
}

void FoilAuthModel::Private::queueSignal(Signal aSignal)
{
    if (aSignal >= 0 && aSignal < SignalCount) {
        const SignalMask signalBit = (SignalMask(1) << aSignal);
        if (iQueuedSignals) {
            iQueuedSignals |= signalBit;
            if (iFirstQueuedSignal > aSignal) {
                iFirstQueuedSignal = aSignal;
            }
        } else {
            iQueuedSignals = signalBit;
            iFirstQueuedSignal = aSignal;
        }
    }
}

void FoilAuthModel::Private::emitQueuedSignals()
{
    static const SignalEmitter emitSignal [] = {
#define FOIL_SIGNAL_EMITTER_(Name,name) &FoilAuthModel::name##Changed,
        FOIL_QUEUED_SIGNALS(FOIL_SIGNAL_EMITTER_)
#undef FOIL_SIGNAL_EMITTER_
    };
    Q_STATIC_ASSERT(G_N_ELEMENTS(emitSignal) == SignalCount);
    if (iQueuedSignals) {
        FoilAuthModel* model = parentModel();
        // Reset first queued signal before emitting the signals.
        // Signal handlers may emit new signals.
        uint i = iFirstQueuedSignal;
        iFirstQueuedSignal = SignalCount;
        for (; i < SignalCount && iQueuedSignals; i++) {
            const SignalMask signalBit = (SignalMask(1) << i);
            if (iQueuedSignals & signalBit) {
                iQueuedSignals &= ~signalBit;
                Q_EMIT (model->*(emitSignal[i]))();
            }
        }
    }
}

inline int FoilAuthModel::Private::rowCount() const
{
    return iData.count();
}

inline FoilAuthModel::ModelData* FoilAuthModel::Private::dataAt(int aIndex) const
{
    if (aIndex >= 0 && aIndex < iData.count()) {
        return iData.at(aIndex);
    } else {
        return NULL;
    }
}

FoilAuthModel::ModelData* FoilAuthModel::Private::findData(QString aId) const
{
    const int n = iData.count();
    for (int i = 0; i < n; i++) {
        ModelData* data = iData.at(i);
        if (data->iId == aId) {
            return data;
        }
    }
    return NULL;
}

int FoilAuthModel::Private::findDataPos(QString aId) const
{
    const int n = iData.count();
    for (int i = 0; i < n; i++) {
        if (iData.at(i)->iId == aId) {
            return i;
        }
    }
    return -1;
}

void FoilAuthModel::Private::setKeys(FoilPrivateKey* aPrivate, FoilKey* aPublic)
{
    if (aPrivate) {
        if (iPrivateKey) {
            foil_private_key_unref(iPrivateKey);
        } else {
            queueSignal(SignalKeyAvailableChanged);
        }
        foil_key_unref(iPublicKey);
        iPrivateKey = foil_private_key_ref(aPrivate);
        iPublicKey = aPublic ? foil_key_ref(aPublic) :
            foil_public_key_new_from_private(aPrivate);
    } else if (iPrivateKey) {
        queueSignal(SignalKeyAvailableChanged);
        foil_private_key_unref(iPrivateKey);
        foil_key_unref(iPublicKey);
        iPrivateKey = NULL;
        iPublicKey = NULL;
    }
}

bool FoilAuthModel::Private::checkPassword(QString aPassword)
{
    GError* error = NULL;
    HDEBUG(iFoilKeyFile);
    const QByteArray path(iFoilKeyFile.toUtf8());

    // First make sure that it's encrypted
    FoilPrivateKey* key = foil_private_key_decrypt_from_file
        (FOIL_KEY_RSA_PRIVATE, path.constData(), NULL, &error);
    if (key) {
        HWARN("Key not encrypted");
        foil_private_key_unref(key);
    } else if (error->domain == FOIL_ERROR) {
        if (error->code == FOIL_ERROR_KEY_ENCRYPTED) {
            // Validate the old password
            QByteArray password(aPassword.toUtf8());
            g_clear_error(&error);
            key = foil_private_key_decrypt_from_file
                (FOIL_KEY_RSA_PRIVATE, path.constData(),
                    password.constData(), &error);
            if (key) {
                HDEBUG("Password OK");
                foil_private_key_unref(key);
                return true;
            } else {
                HDEBUG("Wrong password");
                g_error_free(error);
            }
        } else {
            HWARN("Key invalid:" << error->message);
            g_error_free(error);
        }
    } else {
        HWARN(error->message);
        g_error_free(error);
    }
    return false;
}

bool FoilAuthModel::Private::changePassword(QString aOldPassword,
    QString aNewPassword)
{
    HDEBUG(iFoilKeyFile);
    if (checkPassword(aOldPassword)) {
        GError* error = NULL;
        QByteArray password(aNewPassword.toUtf8());

        // First write the temporary file
        QString tmpKeyFile = iFoilKeyFile + ".new";
        const QByteArray tmp(tmpKeyFile.toUtf8());
        FoilOutput* out = foil_output_file_new_open(tmp.constData());
        if (foil_private_key_encrypt(iPrivateKey, out,
            FOIL_KEY_EXPORT_FORMAT_DEFAULT, password.constData(),
            NULL, &error) && foil_output_flush(out)) {
            foil_output_unref(out);

            // Then rename it
            QString saveKeyFile = iFoilKeyFile + ".save";
            QFile::remove(saveKeyFile);
            if (QFile::rename(iFoilKeyFile, saveKeyFile) &&
                QFile::rename(tmpKeyFile, iFoilKeyFile)) {
                BaseTask::removeFile(saveKeyFile);
                HDEBUG("Password changed");
                Q_EMIT parentModel()->passwordChanged();
                return true;
            }
        } else {
            if (error) {
                HWARN(error->message);
                g_error_free(error);
            }
            foil_output_unref(out);
        }
    }
    return false;
}

void FoilAuthModel::Private::checkTimer()
{
    if (iFoilState == FoilModelReady && !iData.isEmpty()) {
        onTimer();
    } else if (iTimer->isActive()){
        queueSignal(SignalTimerActiveChanged);
        iTimer->stop();
    }
}

void FoilAuthModel::Private::setFoilState(FoilState aState)
{
    if (iFoilState != aState) {
        iFoilState = aState;
        HDEBUG(aState);
        checkTimer();
        queueSignal(SignalFoilStateChanged);
    }
}

void FoilAuthModel::Private::dataChanged(int aIndex, ModelData::Role aRole)
{
    if (aIndex >= 0 && aIndex < iData.count()) {
        QVector<int> roles;
        roles.append(aRole);
        FoilAuthModel* model = parentModel();
        QModelIndex modelIndex(model->index(aIndex));
        Q_EMIT model->dataChanged(modelIndex, modelIndex, roles);
    }
}

void FoilAuthModel::Private::dataChanged(QList<int> aRows, ModelData::Role aRole)
{
    const int n = aRows.count();
    if (n > 0) {
        QVector<int> roles;
        roles.append(aRole);
        FoilAuthModel* model = parentModel();
        for (int i = 0; i < n; i++) {
            const int row = aRows.at(i);
            if (row >= 0 && row < iData.count()) {
                QModelIndex modelIndex(model->index(row));
                Q_EMIT model->dataChanged(modelIndex, modelIndex, roles);
            }
        }
    }
}

void FoilAuthModel::Private::insertModelData(ModelData* aData)
{
    // Insert it into the model
    const int pos = iData.count();
    FoilAuthModel* model = parentModel();
    model->beginInsertRows(QModelIndex(), pos, pos);
    iData.append(aData);
    HDEBUG(aData->iId << aData->iSecretBase32 << aData->label());
    queueSignal(SignalCountChanged);
    if (!pos) checkTimer();
    model->endInsertRows();
}

void FoilAuthModel::Private::onDecryptAllProgress(DecryptAllTask::Progress::Ptr aProgress)
{
    if (aProgress && aProgress->iTask == iDecryptAllTask) {
        // Transfer ownership of this ModelData to the model
        insertModelData(aProgress->iModelData);
        aProgress->iModelData = NULL;
    }
    emitQueuedSignals();
}

void FoilAuthModel::Private::onDecryptAllTaskDone()
{
    HDEBUG(iData.count() << "token(s) decrypted");
    if (sender() == iDecryptAllTask) {
        bool infoUpdated = iDecryptAllTask->iSaveInfo;
        iDecryptAllTask->release();
        iDecryptAllTask = NULL;
        if (iFoilState == FoilDecrypting) {
            setFoilState(FoilModelReady);
        }
        if (infoUpdated) saveInfo();
        if (!busy()) {
            // We know we were busy when we received this signal
            queueSignal(SignalBusyChanged);
        }
    }
    emitQueuedSignals();
}

void FoilAuthModel::Private::addToken(const FoilAuthToken& aToken, bool aFavorite)
{
    QString path = FoilAuth::createEmptyFoilFile(iFoilDataDir);
    ModelData* data = new ModelData(path, aToken, aFavorite);
    insertModelData(data);
    updatePasswords(data);
    encrypt(data);
}

void FoilAuthModel::Private::encrypt(const ModelData* aData)
{
    const bool wasBusy = busy();
    EncryptTask* task = new EncryptTask(iThreadPool, aData,
        iPrivateKey, iPublicKey, iLastTime, iFoilDataDir);
    iEncryptTasks.append(task);
    task->submit(this, SLOT(onEncryptTaskDone()));
    if (!wasBusy) {
        // We must be busy now
        queueSignal(SignalBusyChanged);
    }
}

void FoilAuthModel::Private::onEncryptTaskDone()
{
    EncryptTask* task = qobject_cast<EncryptTask*>(sender());
    if (task) {
        HVERIFY(iEncryptTasks.removeAll(task));
        if (!task->iNewFile.isEmpty()) {
            const int pos = findDataPos(task->iId);
            if (pos >= 0) {
                ModelData* data = iData.at(pos);
                HDEBUG("Encrypted" << qPrintable(task->iNewFile));
                data->setTokenPath(task->iNewFile);

                // Id has definitely changed, passwords may have changed too
                QVector<int> roles;
                roles.append(ModelData::ModelIdRole);
                if (data->iCurrentPassword != task->iCurrentPassword) {
                    data->iCurrentPassword = task->iCurrentPassword;
                    roles.append(ModelData::CurrentPasswordRole);
                }
                if (data->iPrevPassword != task->iPrevPassword) {
                    data->iPrevPassword = task->iPrevPassword;
                    roles.append(ModelData::PrevPasswordRole);
                }
                if (data->iNextPassword != task->iNextPassword) {
                    data->iNextPassword = task->iNextPassword;
                    roles.append(ModelData::NextPasswordRole);
                }
                FoilAuthModel* model = parentModel();
                QModelIndex index(model->index(pos));
                Q_EMIT model->dataChanged(index, index, roles);
                saveInfo();
            }
        }
        task->release();
        if (!busy()) {
            // We know we were busy when we received this signal
            queueSignal(SignalBusyChanged);
        }
        emitQueuedSignals();
    }
}

void FoilAuthModel::Private::updatePasswords(const ModelData* aData)
{
    const bool wasBusy = busy();
    PasswordTask* task = new PasswordTask(iThreadPool, aData, iLastTime);
    iPasswordTasks.append(task);
    task->submit(this, SLOT(onPasswordTaskDone()));
    if (!wasBusy) {
        // We must be busy now
        queueSignal(SignalBusyChanged);
    }
}

void FoilAuthModel::Private::onPasswordTaskDone()
{
    PasswordTask* task = qobject_cast<PasswordTask*>(sender());
    if (task) {
        HVERIFY(iPasswordTasks.removeAll(task));
        const int pos = findDataPos(task->iId);
        if (pos >= 0) {
            ModelData* data = iData.at(pos);
            QVector<int> roles;
            if (data->iPrevPassword != task->iPrevPassword) {
                data->iPrevPassword = task->iPrevPassword;
                roles.append(ModelData::PrevPasswordRole);
            }
            if (data->iCurrentPassword != task->iCurrentPassword) {
                data->iCurrentPassword = task->iCurrentPassword;
                roles.append(ModelData::CurrentPasswordRole);
            }
            if (data->iNextPassword != task->iNextPassword) {
                data->iNextPassword = task->iNextPassword;
                roles.append(ModelData::NextPasswordRole);
            }
            if (roles.size() > 0) {
                HDEBUG("Updated" << qPrintable(data->label()));
                FoilAuthModel* model = parentModel();
                QModelIndex modelIndex(model->index(pos));
                Q_EMIT model->dataChanged(modelIndex, modelIndex, roles);
            }
        }
        task->release();
        if (!busy()) {
            // We know we were busy when we received this signal
            queueSignal(SignalBusyChanged);
        }
        emitQueuedSignals();
    }
}

void FoilAuthModel::Private::saveInfo()
{
    // N.B. This method may change the busy state but doesn't queue
    // BusyChanged signal, it's done by the caller.
    if (iSaveInfoTask) iSaveInfoTask->release();
    iSaveInfoTask = new SaveInfoTask(iThreadPool, ModelInfo(iData),
        iFoilDataDir, iPrivateKey, iPublicKey);
    iSaveInfoTask->submit(this, SLOT(onSaveInfoDone()));
}

void FoilAuthModel::Private::onSaveInfoDone()
{
    HDEBUG("Done");
    if (sender() == iSaveInfoTask) {
        iSaveInfoTask->release();
        iSaveInfoTask = NULL;
        if (!busy()) {
            // We know we were busy when we received this signal
            queueSignal(SignalBusyChanged);
        }
        emitQueuedSignals();
    }
}

void FoilAuthModel::Private::generate(int aBits, QString aPassword)
{
    const bool wasBusy = busy();
    if (iGenerateKeyTask) iGenerateKeyTask->release();
    iGenerateKeyTask = new GenerateKeyTask(iThreadPool, iFoilKeyFile,
        aBits, aPassword);
    iGenerateKeyTask->submit(this, SLOT(onGenerateKeyTaskDone()));
    setFoilState(FoilGeneratingKey);
    if (!wasBusy) {
        // We know we are busy now
        queueSignal(SignalBusyChanged);
    }
    emitQueuedSignals();
}

void FoilAuthModel::Private::onGenerateKeyTaskDone()
{
    HDEBUG("Got a new key");
    HASSERT(sender() == iGenerateKeyTask);
    if (iGenerateKeyTask->iPrivateKey) {
        setKeys(iGenerateKeyTask->iPrivateKey, iGenerateKeyTask->iPublicKey);
        setFoilState(FoilModelReady);
    } else {
        setKeys(NULL);
        setFoilState(FoilKeyError);
    }
    iGenerateKeyTask->release();
    iGenerateKeyTask = NULL;
    if (!busy()) {
        // We know we were busy when we received this signal
        queueSignal(SignalBusyChanged);
    }
    Q_EMIT parentModel()->keyGenerated();
    emitQueuedSignals();
}

void FoilAuthModel::Private::destroyItemAt(int aIndex)
{
    if (aIndex >= 0 && aIndex <= iData.count()) {
        FoilAuthModel* model = parentModel();
        HDEBUG(iData.at(aIndex)->label());
        model->beginRemoveRows(QModelIndex(), aIndex, aIndex);
        delete iData.takeAt(aIndex);
        model->endRemoveRows();
        queueSignal(SignalCountChanged);
        if (iData.isEmpty()) checkTimer();
    }
}

bool FoilAuthModel::Private::destroyItemAndRemoveFilesAt(int aIndex)
{
    ModelData* data = dataAt(aIndex);
    if (data) {
        QString path(data->iPath);
        destroyItemAt(aIndex);
        BaseTask::removeFile(path);
        return true;
    }
    return false;
}

bool FoilAuthModel::Private::destroyLastItemAndRemoveFiles()
{
    const int n = iData.count();
    if (n > 0) {
        ModelData* data = iData.at(n - 1);
        QString path(data->iPath);
        destroyItemAt(n - 1);
        BaseTask::removeFile(path);
        return true;
    }
    return false;
}

void FoilAuthModel::Private::deleteToken(QString aId)
{
    const bool wasBusy = busy();
    if (destroyItemAndRemoveFilesAt(findDataPos(aId))) {
        // saveInfo() doesn't queue BusyChanged signal, we have to do it here
        saveInfo();
        if (wasBusy != busy()) {
            queueSignal(SignalBusyChanged);
        }
    } else {
        HDEBUG("Invalid token id" << aId);
    }
}

void FoilAuthModel::Private::deleteAll()
{
    const int n = iData.count();
    if (n > 0) {
        const bool wasBusy = busy();
        FoilAuthModel* model = parentModel();
        model->beginRemoveRows(QModelIndex(), 0, n - 1);
        while (destroyLastItemAndRemoveFiles());
        model->endRemoveRows();
        queueSignal(SignalCountChanged);
        checkTimer();
        saveInfo();
        // saveInfo() doesn't queue BusyChanged signal, we have to do it here
        if (wasBusy != busy()) {
            queueSignal(SignalBusyChanged);
        }
    }
}

void FoilAuthModel::Private::clearModel()
{
    const int n = iData.count();
    if (n > 0) {
        FoilAuthModel* model = parentModel();
        model->beginRemoveRows(QModelIndex(), 0, n - 1);
        qDeleteAll(iData);
        iData.clear();
        model->endRemoveRows();
        queueSignal(SignalCountChanged);
        checkTimer();
    }
}

void FoilAuthModel::Private::lock(bool aTimeout)
{
    // Cancel whatever we are doing
    FoilAuthModel* model = parentModel();
    const bool wasBusy = busy();
    if (iSaveInfoTask) {
        iSaveInfoTask->release();
        iSaveInfoTask = NULL;
    }
    if (iDecryptAllTask) {
        iDecryptAllTask->release();
        iDecryptAllTask = NULL;
    }
    QList<EncryptTask*> encryptTasks(iEncryptTasks);
    iEncryptTasks.clear();
    int i;
    for (i = 0; i < encryptTasks.count(); i++) {
        EncryptTask* task = encryptTasks.at(i);
        task->release();
    }
    // Destroy decrypted notes
    if (!iData.isEmpty()) {
        model->beginRemoveRows(QModelIndex(), 0, iData.count() - 1);
        qDeleteAll(iData);
        iData.clear();
        model->endRemoveRows();
        queueSignal(SignalCountChanged);
        checkTimer();
    }
    if (busy() != wasBusy) {
        queueSignal(SignalBusyChanged);
    }
    if (iPrivateKey) {
        // Throw the keys away
        setKeys(NULL);
        setFoilState(aTimeout ? FoilLockedTimedOut : FoilLocked);
        HDEBUG("Locked");
    } else {
        HDEBUG("Nothing to lock, there's no key yet!");
    }
}

bool FoilAuthModel::Private::unlock(QString aPassword)
{
    GError* error = NULL;
    HDEBUG(iFoilKeyFile);
    const QByteArray path(iFoilKeyFile.toUtf8());
    bool ok = false;

    // First make sure that it's encrypted
    FoilPrivateKey* key = foil_private_key_decrypt_from_file
        (FOIL_KEY_RSA_PRIVATE, path.constData(), NULL, &error);
    if (key) {
        HWARN("Key not encrypted");
        setFoilState(FoilKeyNotEncrypted);
        foil_private_key_unref(key);
    } else if (error->domain == FOIL_ERROR) {
        if (error->code == FOIL_ERROR_KEY_ENCRYPTED) {
            // Then try to decrypt it
            const QByteArray password(aPassword.toUtf8());
            g_clear_error(&error);
            key = foil_private_key_decrypt_from_file
                (FOIL_KEY_RSA_PRIVATE, path.constData(),
                    password.constData(), &error);
            if (key) {
                HDEBUG("Password accepted, thank you!");
                setKeys(key);
                // Now that we know the key, decrypt the tokens
                if (iDecryptAllTask) iDecryptAllTask->release();
                iDecryptAllTask = new DecryptAllTask(iThreadPool,
                    iFoilDataDir, iPrivateKey, iPublicKey);
                clearModel();
                connect(iDecryptAllTask,
                    SIGNAL(progress(DecryptAllTask::Progress::Ptr)),
                    SLOT(onDecryptAllProgress(DecryptAllTask::Progress::Ptr)),
                    Qt::QueuedConnection);
                iDecryptAllTask->submit(this, SLOT(onDecryptAllTaskDone()));
                setFoilState(FoilDecrypting);
                foil_private_key_unref(key);
                ok = true;
            } else {
                HDEBUG("Wrong password");
                g_error_free(error);
                setFoilState(FoilLocked);
            }
        } else {
            HWARN("Key invalid:" << error->message);
            g_error_free(error);
            setFoilState(FoilKeyInvalid);
        }
    } else {
        HWARN(error->message);
        g_error_free(error);
        setFoilState(FoilKeyMissing);
    }
    return ok;
}

bool FoilAuthModel::Private::busy() const
{
    if (iSaveInfoTask ||
        iGenerateKeyTask ||
        iDecryptAllTask ||
        !iEncryptTasks.isEmpty() ||
        !iPasswordTasks.isEmpty()) {
        return true;
    } else {
        return false;
    }
}

void FoilAuthModel::Private::onTimer()
{
    const QDateTime now(QDateTime::currentDateTime());
    int msec = 1000 - now.time().msec();
    if (msec < 5) msec += 1000;
    if (!iTimer->isActive()) {
        queueSignal(SignalTimerActiveChanged);
    }
    iTimer->start(msec);
    iLastTime = now.toTime_t();
    const quint64 thisPeriod = iLastTime / FoilAuth::PERIOD;
    const uint timeLeft = FoilAuth::PERIOD - (iLastTime % FoilAuth::PERIOD);
    if (iLastPeriod != thisPeriod) {
        iLastPeriod = thisPeriod;
        iTimeLeft = FoilAuth::PERIOD;
        queueSignal(SignalTimeLeftChanged);
        const int n = iData.count();
        for (int i = 0; i < n; i++) {
            updatePasswords(iData.at(i));
        }
        Q_EMIT parentModel()->timerRestarted();
    } else if (iTimeLeft != timeLeft) {
        iTimeLeft = timeLeft;
        queueSignal(SignalTimeLeftChanged);
    }
    emitQueuedSignals();
    if (iTimeLeft != timeLeft) {
        iTimeLeft = timeLeft;
        Q_EMIT parentModel()->timeLeftChanged();
    }
}

// ==========================================================================
// FoilAuthModel
// ==========================================================================

#define SUPER QAbstractListModel

FoilAuthModel::FoilAuthModel(QObject* aParent) :
    SUPER(aParent),
    iPrivate(new Private(this))
{
}

// Callback for qmlRegisterSingletonType<FoilAuthModel>
QObject* FoilAuthModel::createSingleton(QQmlEngine* aEngine, QJSEngine* aScript)
{
    return new FoilAuthModel(aEngine);
}

int FoilAuthModel::favoriteRole()
{
    return ModelData::FavoriteRole;
}

Qt::ItemFlags FoilAuthModel::flags(const QModelIndex& aIndex) const
{
    return SUPER::flags(aIndex) | Qt::ItemIsEditable;
}

QHash<int,QByteArray> FoilAuthModel::roleNames() const
{
    QHash<int,QByteArray> roles;
#define ROLE(X,x) roles.insert(ModelData::X##Role, #x);
FOILAUTH_ROLES(ROLE)
#undef ROLE
    return roles;
}

int FoilAuthModel::rowCount(const QModelIndex& aParent) const
{
    return iPrivate->rowCount();
}

QVariant FoilAuthModel::data(const QModelIndex& aIndex, int aRole) const
{
    ModelData* data = iPrivate->dataAt(aIndex.row());
    return data ? data->get((ModelData::Role)aRole) : QVariant();
}

bool FoilAuthModel::setData(const QModelIndex& aIndex, const QVariant& aValue, int aRole)
{
    const int row = aIndex.row();
    ModelData* data = iPrivate->dataAt(row);
    if (data) {
        QVector<int> roles;
        switch ((ModelData::Role)aRole) {
        case ModelData::FavoriteRole:
            {
                const bool favorite = aValue.toBool();
                HDEBUG(row << "favorite" << favorite);
                if (data->iFavorite != favorite) {
                    data->iFavorite = favorite;
                    iPrivate->encrypt(data);
                    iPrivate->emitQueuedSignals();
                    roles.append(aRole);
                    Q_EMIT dataChanged(aIndex, aIndex, roles);
                }
            }
            return true;
        case ModelData::LabelRole:
            {
                const QString label = aValue.toString();
                HDEBUG(row << "label" << label);
                if (data->label() != label) {
                    data->iToken.iLabel = label;
                    iPrivate->encrypt(data);
                    iPrivate->emitQueuedSignals();
                    roles.append(aRole);
                    Q_EMIT dataChanged(aIndex, aIndex, roles);
                }
            }
            return true;
        case ModelData::SecretRole:
            {
                QString base32(aValue.toString().toLower());
                const QByteArray secret(FoilAuth::fromBase32(base32));
                HDEBUG(row << "secret" << base32 << "->" << secret.size() << "bytes");
                if (secret.size() > 0) {
                    base32 = FoilAuth::toBase32(secret);
                    if (data->iToken.iBytes != secret) {
                        // Secret has actually changed
                        data->iToken.iBytes = secret;
                        data->iSecretBase32 = base32;
                        iPrivate->encrypt(data);
                        iPrivate->emitQueuedSignals();
                        roles.append(aRole);
                        Q_EMIT dataChanged(aIndex, aIndex, roles);
                    } else if (data->iSecretBase32 != base32) {
                        // Fix BASE32 representation
                        data->iSecretBase32 = base32;
                        roles.append(aRole);
                        Q_EMIT dataChanged(aIndex, aIndex, roles);
                    }
                    return true;
                }
            }
            break;
        case ModelData::DigitsRole:
            {
                bool ok;
                const int digits = aValue.toInt(&ok);
                HDEBUG(row << "digits" << digits);
                if (ok) {
                    if (data->iToken.iDigits == digits) {
                        return true;
                    } else if (data->iToken.setDigits(digits)) {
                        iPrivate->encrypt(data);
                        iPrivate->emitQueuedSignals();
                        roles.append(aRole);
                        Q_EMIT dataChanged(aIndex, aIndex, roles);
                        return true;
                    }
                }
            }
            break;
        case ModelData::TimeShiftRole:
            {
                bool ok;
                int sec = aValue.toInt(&ok);
                HDEBUG(row << "timeshift" << sec);
                if (ok) {
                    if (data->iToken.iTimeShift != sec) {
                        data->iToken.iTimeShift = sec;
                        iPrivate->encrypt(data);
                        iPrivate->emitQueuedSignals();
                        roles.append(aRole);
                        Q_EMIT dataChanged(aIndex, aIndex, roles);
                    }
                    return true;
                }
            }
        break;
        // No default to make sure that we get "warning: enumeration value
        // not handled in switch" if we forget to handle a real role.
        case ModelData::ModelIdRole:
        case ModelData::IssuerRole:
        case ModelData::PrevPasswordRole:
        case ModelData::CurrentPasswordRole:
        case ModelData::NextPasswordRole:
            HDEBUG(row << aRole << "nope");
            break;
        }
    }
    return false;
}

bool FoilAuthModel::moveRows(const QModelIndex &aSrcParent, int aSrcRow,
    int aCount, const QModelIndex &aDestParent, int aDestRow)
{
    const int size = iPrivate->rowCount();
    if (aSrcParent == aDestParent &&
        aSrcRow != aDestRow &&
        aSrcRow >= 0 && aSrcRow < size &&
        aDestRow >= 0 && aDestRow < size) {

        HDEBUG(aSrcRow << "->" << aDestRow);
        const bool wasBusy = iPrivate->busy();
        beginMoveRows(aSrcParent, aSrcRow, aSrcRow, aDestParent,
           (aDestRow < aSrcRow) ? aDestRow : (aDestRow + 1));
        iPrivate->iData.move(aSrcRow, aDestRow);
        endMoveRows();

        iPrivate->saveInfo();
        if (!wasBusy) {
            iPrivate->queueSignal(Private::SignalBusyChanged);
        }
        iPrivate->emitQueuedSignals();
        return true;
    } else {
        return false;
    }
}

int FoilAuthModel::period() const
{
    return FoilAuth::PERIOD;
}

int FoilAuthModel::timeLeft() const
{
    return iPrivate->iTimeLeft;
}

bool FoilAuthModel::busy() const
{
    return iPrivate->busy();
}

bool FoilAuthModel::keyAvailable() const
{
    return iPrivate->iPrivateKey != NULL;
}

bool FoilAuthModel::timerActive() const
{
    return iPrivate->iTimer->isActive();
}

FoilAuthModel::FoilState FoilAuthModel::foilState() const
{
    return iPrivate->iFoilState;
}

bool FoilAuthModel::checkPassword(QString aPassword)
{
    return iPrivate->checkPassword(aPassword);
}

bool FoilAuthModel::changePassword(QString aOld, QString aNew)
{
    bool ok = iPrivate->changePassword(aOld, aNew);
    iPrivate->emitQueuedSignals();
    return ok;
}

void FoilAuthModel::generateKey(int aBits, QString aPassword)
{
    iPrivate->generate(aBits, aPassword);
    iPrivate->emitQueuedSignals();
}

void FoilAuthModel::lock(bool aTimeout)
{
    iPrivate->lock(aTimeout);
    iPrivate->emitQueuedSignals();
}

bool FoilAuthModel::unlock(QString aPassword)
{
    const bool ok = iPrivate->unlock(aPassword);
    iPrivate->emitQueuedSignals();
    return ok;
}

int FoilAuthModel::millisecondsLeft()
{
    if (iPrivate->iTimeLeft > 0) {
        const QDateTime now(QDateTime::currentDateTime());
        const int msec = now.time().msec();
        if (msec > 0) {
            return (iPrivate->iTimeLeft - 1) * 1000 + msec;
        } else {
            return iPrivate->iTimeLeft * 1000;
        }
    } else {
        return 0;
    }
}

bool FoilAuthModel::addToken(QString aSecretBase32, QString aLabel,
    QString aIssuer, int aDigits, int aTimeShift)
{
    HDEBUG(aSecretBase32 << aLabel << aIssuer << aDigits << aTimeShift);
    QByteArray secretBytes = FoilAuth::fromBase32(aSecretBase32);
    if (secretBytes.size() > 0) {
        FoilAuthToken token(secretBytes, aLabel, aIssuer, aDigits, aTimeShift);
        iPrivate->addToken(token);
        iPrivate->emitQueuedSignals();
        return true;
    }
    return false;
}

void FoilAuthModel::addToken(const FoilAuthToken* aToken, bool aFavorite)
{
    if (aToken && aToken->isValid()) {
        HDEBUG(aToken->iLabel);
        iPrivate->addToken(*aToken, aFavorite);
        iPrivate->emitQueuedSignals();
    }
}

bool FoilAuthModel::addTokenUri(QString aUri)
{
    FoilAuthToken token;
    HDEBUG(aUri);
    if (token.parseUri(aUri)) {
        iPrivate->addToken(token);
        iPrivate->emitQueuedSignals();
        return true;
    }
    return false;
}

void FoilAuthModel::deleteToken(QString aId)
{
    HDEBUG(aId);
    iPrivate->deleteToken(aId);
    iPrivate->emitQueuedSignals();
}

void FoilAuthModel::deleteAll()
{
    HDEBUG("deleting all tokena");
    iPrivate->deleteAll();
    iPrivate->emitQueuedSignals();
}

int FoilAuthModel::indexOf(const FoilAuthToken* aToken) const
{
    if (aToken) {
        const int n = iPrivate->rowCount();
        for (int i = 0; i < n; i++) {
            if (iPrivate->dataAt(i)->iToken.equals(aToken)) {
                return i;
            }
        }
    }
    return -1;
}

#include "FoilAuthModel.moc"
