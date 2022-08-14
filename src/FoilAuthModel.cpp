/*
 * Copyright (C) 2019-2022 Jolla Ltd.
 * Copyright (C) 2019-2022 Slava Monich <slava@monich.com>
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

#include "HarbourBase32.h"
#include "HarbourDebug.h"
#include "HarbourTask.h"

#include "foil_output.h"
#include "foil_private_key.h"
#include "foil_random.h"
#include "foil_util.h"

#include "foilmsg.h"

#include "gutil_misc.h"

#include <QDir>
#include <QFile>
#include <QTimer>
#include <QFileInfo>
#include <QDateTime>
#include <QThreadPool>
#include <QSharedPointer>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define ENCRYPT_FILE_MODE       0600

#define ENCRYPT_KEY_TYPE        FOILMSG_KEY_AES_256
#define SIGNATURE_TYPE          FOILMSG_SIGNATURE_SHA256_RSA

#define HEADER_LABEL            "OTP-Label"
#define HEADER_ISSUER           "OTP-Issuer"
#define HEADER_TYPE             "OTP-Type"
#define HEADER_ALGORITHM        "OTP-Algorithm"
#define HEADER_DIGITS           "OTP-Digits"
#define HEADER_COUNTER          "OTP-Counter"    // HOTP specific
#define HEADER_TIMESHIFT        "OTP-TimeShift"  // TOTP specific
#define HEADER_FAVORITE         "OTP-Favorite"
#define MAX_HEADERS             8

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
    role(Type,type) \
    role(Secret,secret) \
    role(Issuer,issuer) \
    role(Digits,digits) \
    role(Algorithm,algorithm) \
    role(Counter,counter) \
    role(Timeshift,timeshift) \
    role(Label,label) \
    role(PrevPassword,prevPassword) \
    role(CurrentPassword,currentPassword) \
    last(NextPassword,nextPassword)

#define FOILAUTH_ROLES(role) \
    FOILAUTH_ROLES_(role,role,role)

// ==========================================================================
// FoilAuthModel::Util
// ==========================================================================

class FoilAuthModel::Util {
private:
    Util();
public:
    static const FoilMsgEncryptOptions* encryptionOptions(FoilMsgEncryptOptions*);
};


const FoilMsgEncryptOptions*
FoilAuthModel::Util::encryptionOptions(
    FoilMsgEncryptOptions* aOpt)
{
    foilmsg_encrypt_defaults(aOpt);
    aOpt->key_type = ENCRYPT_KEY_TYPE;
    aOpt->signature = SIGNATURE_TYPE;
    return aOpt;
}

// ==========================================================================
// FoilAuthModel::ModelData
// ==========================================================================

class FoilAuthModel::ModelData :
    public FoilAuthTypes
{
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

    typedef QList<ModelData*> List;

    ModelData(const QString aPath, AuthType aType,
        const QByteArray aSecret, const QString aLabel, const QString aIssuer,
        int aDigits = DEFAULT_DIGITS, int aCounter = DEFAULT_COUNTER,
        int aTimeShift = DEFAULT_TIMESHIFT,
        DigestAlgorithm aAlgorithm = DEFAULT_ALGORITHM,
        bool aFavorite = true);
    ModelData(const QString, const FoilAuthToken&, bool aFavorite = true);

    QVariant get(Role aRole) const;
    const QString label() { return iToken.label(); }
    void setTokenPath(QString aPath);

    static AuthType headerAuthType(const FoilMsg*);
    static DigestAlgorithm headerAlgorithm(const FoilMsg*);
    static QString headerString(const FoilMsg*, const char*);
    static quint64 headerUint64(const FoilMsg*, const char*, quint64);
    static int headerInt(const FoilMsg*, const char*, int);
    static bool headerBool(const FoilMsg*, const char*, bool);

public:
    QString iPath;
    QString iId;
    bool iFavorite;
    FoilAuthToken iToken;
    QString iPrevPassword;
    QString iCurrentPassword;
    QString iNextPassword;
};

FoilAuthModel::ModelData::ModelData(
    const QString aPath,
    AuthType aType,
    const QByteArray aSecret,
    const QString aLabel,
    const QString aIssuer,
    int aDigits,
    int aCounter,
    int aTimeShift,
    DigestAlgorithm aAlgorithm,
    bool aFavorite) :
    iPath(aPath),
    iId(QFileInfo(aPath).fileName()),
    iFavorite(aFavorite),
    iToken(aType, aSecret, aLabel, aIssuer, aDigits, aCounter, aTimeShift, aAlgorithm)
{
    HDEBUG(iToken.secretBase32() << iToken.label());
}

FoilAuthModel::ModelData::ModelData(
    const QString aPath,
    const FoilAuthToken& aToken,
    bool aFavorite) :
    iPath(aPath),
    iId(QFileInfo(aPath).fileName()),
    iFavorite(aFavorite),
    iToken(aToken)
{
    HDEBUG(iToken.secretBase32() << iToken.label());
}

void
FoilAuthModel::ModelData::setTokenPath(
    QString aPath)
{
    iPath = aPath;
    iId = QFileInfo(aPath).fileName();
}

QVariant
FoilAuthModel::ModelData::get(
    Role aRole) const
{
    switch (aRole) {
    case ModelIdRole: return iId;
    case FavoriteRole: return iFavorite;
    case SecretRole: return iToken.secretBase32();
    case IssuerRole: return iToken.issuer();
    case DigitsRole: return iToken.digits();
    case TypeRole: return (int)iToken.type();
    case AlgorithmRole: return (int)iToken.algorithm();
    case CounterRole: return iToken.counter();
    case TimeshiftRole: return iToken.timeshift();
    case LabelRole: return iToken.label();
    case PrevPasswordRole: return iPrevPassword;
    case CurrentPasswordRole: return iCurrentPassword;
    case NextPasswordRole: return iNextPassword;
    }
    return QVariant();
}

FoilAuthTypes::AuthType
FoilAuthModel::ModelData::headerAuthType(
    const FoilMsg* aMsg)
{
    const char* value = foilmsg_get_value(aMsg, HEADER_TYPE);
    if (value) {
        if (!g_ascii_strcasecmp(value, FOILAUTH_TYPE_TOTP)) {
            return AuthTypeTOTP;
        } else if (!g_ascii_strcasecmp(value, FOILAUTH_TYPE_HOTP)) {
            return AuthTypeHOTP;
        }
    }
    return DEFAULT_AUTH_TYPE;
}

FoilAuthTypes::DigestAlgorithm
FoilAuthModel::ModelData::headerAlgorithm(
    const FoilMsg* aMsg)
{
    const char* value = foilmsg_get_value(aMsg, HEADER_ALGORITHM);
    if (value) {
        if (!g_ascii_strcasecmp(value, FOILAUTH_ALGORITHM_SHA1)) {
            return DigestAlgorithmSHA1;
        } else if (!g_ascii_strcasecmp(value, FOILAUTH_ALGORITHM_SHA256)) {
            return DigestAlgorithmSHA256;
        } else if (!g_ascii_strcasecmp(value, FOILAUTH_ALGORITHM_SHA512)) {
            return DigestAlgorithmSHA512;
        }
    }
    return DEFAULT_ALGORITHM;
}

QString
FoilAuthModel::ModelData::headerString(
    const FoilMsg* aMsg,
    const char* aKey)
{
    const char* value = foilmsg_get_value(aMsg, aKey);
    return (value && value[0]) ? QString::fromUtf8(value) : QString();
}

quint64
FoilAuthModel::ModelData::headerUint64(
    const FoilMsg* aMsg,
    const char* aKey,
    quint64 aDefault)
{
    const char* str = foilmsg_get_value(aMsg, aKey);
    guint64 value = aDefault;
    gutil_parse_uint64(str, 10, &value);
    return value;
}

int
FoilAuthModel::ModelData::headerInt(
    const FoilMsg* aMsg,
    const char* aKey,
    int aDefault)
{
    const char* str = foilmsg_get_value(aMsg, aKey);
    int value = aDefault;
    gutil_parse_int(str, 10, &value);
    return value;
}

bool
FoilAuthModel::ModelData::headerBool(
    const FoilMsg* aMsg,
    const char* aKey,
    bool aDefault)
{
    const char* str = foilmsg_get_value(aMsg, aKey);
    int value = 0;
    return gutil_parse_int(str, 10, &value) && value;
}

// ==========================================================================
// FoilNotesModel::ModelInfo
// ==========================================================================

class FoilAuthModel::ModelInfo
{
public:
    ModelInfo() {}
    ModelInfo(const FoilMsg*);
    ModelInfo(const ModelInfo&);
    ModelInfo(const ModelData::List);

    static ModelInfo load(const QString, FoilPrivateKey*, FoilKey*);

    void save(const QString aDir, FoilPrivateKey*, FoilKey*);
    ModelInfo& operator = (const ModelInfo&);

public:
    QStringList iOrder;
};

FoilAuthModel::ModelInfo::ModelInfo(
    const ModelInfo& aInfo) :
    iOrder(aInfo.iOrder)
{
}

FoilAuthModel::ModelInfo::ModelInfo(
const ModelData::List aData)
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

FoilAuthModel::ModelInfo::ModelInfo(
    const FoilMsg* aMsg)
{
    const char* order = foilmsg_get_value(aMsg, INFO_ORDER_HEADER);
    if (order) {
        char** strv = g_strsplit(order, INFO_ORDER_DELIMITER_S, -1);
        for (char** ptr = strv; *ptr; ptr++) {
            iOrder.append(g_strstrip(*ptr));
        }
        g_strfreev(strv);
        HDEBUG(order);
    }
}

FoilAuthModel::ModelInfo&
FoilAuthModel::ModelInfo::operator=(
    const ModelInfo& aInfo)
{
    iOrder = aInfo.iOrder;
    return *this;
}

FoilAuthModel::ModelInfo
FoilAuthModel::ModelInfo::load(
    const QString aDir,
    FoilPrivateKey* aPrivate,
    FoilKey* aPublic)
{
    ModelInfo info;
    const QString fullPath(aDir + "/" INFO_FILE);
    const QByteArray path(fullPath.toUtf8());
    const char* fname = path.constData();
    HDEBUG("Loading" << fname);
    FoilMsg* aMsg = foilmsg_decrypt_file(aPrivate, fname, NULL);
    if (aMsg) {
        if (foilmsg_verify(aMsg, aPublic)) {
            info = ModelInfo(aMsg);
        } else {
            HWARN("Could not verify" << fname);
        }
        foilmsg_free(aMsg);
    }
    return info;
}

void
FoilAuthModel::ModelInfo::save(
    const QString aDir,
    FoilPrivateKey* aPrivate,
    FoilKey* aPublic)
{
    const QString fullPath(aDir + "/" INFO_FILE);
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

        FoilBytes data;
        FoilMsgEncryptOptions opt;
        foil_bytes_from_string(&data, INFO_CONTENTS);
        foilmsg_encrypt(out, &data, NULL, &headers, aPrivate, aPublic,
            Util::encryptionOptions(&opt), NULL);
        foil_output_unref(out);
    } else {
        HWARN("Failed to open" << fname);
    }
}

// ==========================================================================
// FoilAuthModel::BaseTask
// ==========================================================================

class FoilAuthModel::BaseTask :
    public HarbourTask,
    public FoilAuthTypes
{
    Q_OBJECT

public:
    BaseTask(QThreadPool*, FoilPrivateKey*, FoilKey*);
    virtual ~BaseTask();

    FoilMsg* decryptAndVerify(const QString) const;
    FoilMsg* decryptAndVerify(const char*) const;

    static bool removeFile(const QString);
    static QByteArray toByteArray(const FoilMsg*);

public:
    FoilPrivateKey* iPrivateKey;
    FoilKey* iPublicKey;
};

FoilAuthModel::BaseTask::BaseTask(
    QThreadPool* aPool,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey) :
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

bool
FoilAuthModel::BaseTask::removeFile(
    const QString aPath)
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

FoilMsg*
FoilAuthModel::BaseTask::decryptAndVerify(
    const QString aFileName) const
{
    if (!aFileName.isEmpty()) {
        const QByteArray fileNameBytes(aFileName.toUtf8());
        return decryptAndVerify(fileNameBytes.constData());
    } else{
        return NULL;
    }
}

FoilMsg*
FoilAuthModel::BaseTask::decryptAndVerify(
    const char* aFileName) const
{
    if (aFileName) {
        HDEBUG("Decrypting" << aFileName);
        FoilMsg* aMsg = foilmsg_decrypt_file(iPrivateKey, aFileName, NULL);
        if (aMsg) {
#if HARBOUR_DEBUG
            for (uint i = 0; i < aMsg->headers.count; i++) {
                const FoilMsgHeader* header = aMsg->headers.header + i;
                HDEBUG(" " << header->name << ":" << header->value);
            }
#endif // HARBOUR_DEBUG
            if (foilmsg_verify(aMsg, iPublicKey)) {
                return aMsg;
            } else {
                HWARN("Could not verify" << aFileName);
            }
            foilmsg_free(aMsg);
        }
    }
    return NULL;
}

QByteArray
FoilAuthModel::BaseTask::toByteArray(
    const FoilMsg* aMsg)
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

class FoilAuthModel::GenerateKeyTask :
public BaseTask
{
    Q_OBJECT

public:
    GenerateKeyTask(QThreadPool*, const QString, int, const QString);

    void performTask() Q_DECL_OVERRIDE;

public:
    const QString iKeyFile;
    const int iBits;
    const QString iPassword;
};

FoilAuthModel::GenerateKeyTask::GenerateKeyTask(
    QThreadPool* aPool,
    const QString aKeyFile,
    int aBits,
    const QString aPassword) :
    BaseTask(aPool, NULL, NULL),
    iKeyFile(aKeyFile),
    iBits(aBits),
    iPassword(aPassword)
{
}

void
FoilAuthModel::GenerateKeyTask::performTask()
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

class FoilAuthModel::EncryptTask :
    public BaseTask
{
    Q_OBJECT

public:
    EncryptTask(QThreadPool*, const ModelData*, FoilPrivateKey*, FoilKey*,
        quint64, const QString);

    void performTask() Q_DECL_OVERRIDE;

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

FoilAuthModel::EncryptTask::EncryptTask(
    QThreadPool* aPool,
    const ModelData* aData,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey,
    quint64 aTime,
    const QString aDestDir) :
    BaseTask(aPool, aPrivateKey, aPublicKey),
    iId(aData->iId),
    iFavorite(aData->iFavorite),
    iToken(aData->iToken),
    iDestDir(aDestDir),
    iRemoveFile(aData->iPath),
    iTime(aTime)
{
    HDEBUG("Encrypting" << iToken.label());
}

void
FoilAuthModel::EncryptTask::performTask()
{
    GString* dest = g_string_sized_new(iDestDir.size() + 9);
    FoilOutput* out = FoilAuth::createFoilFile(iDestDir, dest);
    if (out) {
        FoilMsgHeaders headers;
        FoilMsgHeader header[MAX_HEADERS];

        headers.header = header;
        headers.count = 0;

        if (iToken.type() != DEFAULT_AUTH_TYPE) {
            header[headers.count].name = HEADER_TYPE;
            header[headers.count].value = FOILAUTH_TYPE_DEFAULT;
            switch (iToken.type()) {
            case AuthTypeTOTP:
                header[headers.count].value = FOILAUTH_TYPE_TOTP;
                break;
            case AuthTypeHOTP:
                header[headers.count].value = FOILAUTH_TYPE_HOTP;
                break;
            }
            headers.count++;
        }

        const QByteArray label(iToken.label().toUtf8());
        header[headers.count].name = HEADER_LABEL;
        header[headers.count].value = label.constData();
        headers.count++;

        QByteArray issuer;
        if (!iToken.issuer().isEmpty()) {
            issuer = iToken.issuer().toUtf8();
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
        snprintf(digits, sizeof(digits), "%d", iToken.digits());
        header[headers.count].name = HEADER_DIGITS;
        header[headers.count].value = digits;
        headers.count++;

        char timeshift[16];
        if (iToken.timeshift() != DEFAULT_TIMESHIFT) {
            snprintf(timeshift, sizeof(timeshift), "%d", iToken.timeshift());
            header[headers.count].name = HEADER_TIMESHIFT;
            header[headers.count].value = timeshift;
            headers.count++;
        }

        char counter[16];
        if (iToken.counter() != DEFAULT_COUNTER) {
            snprintf(counter, sizeof(counter), "%llu", iToken.counter());
            header[headers.count].name = HEADER_COUNTER;
            header[headers.count].value = counter;
            headers.count++;
        }

        if (iToken.algorithm() != DEFAULT_ALGORITHM) {
            header[headers.count].name = HEADER_ALGORITHM;
            header[headers.count].value = FOILAUTH_ALGORITHM_DEFAULT;
            switch (iToken.algorithm()) {
            case DigestAlgorithmSHA1:
                header[headers.count].value = FOILAUTH_ALGORITHM_SHA1;
                break;
            case DigestAlgorithmSHA256:
                header[headers.count].value = FOILAUTH_ALGORITHM_SHA256;
                break;
            case DigestAlgorithmSHA512:
                header[headers.count].value = FOILAUTH_ALGORITHM_SHA512;
                break;
            }
            headers.count++;
        }

        HASSERT(headers.count <= G_N_ELEMENTS(header));
        HDEBUG("Writing" << iToken);

        const QByteArray secret(iToken.secret());
        FoilBytes body;
        body.val = (guint8*)secret.constData();
        body.len = secret.length();

        FoilMsgEncryptOptions opt;
        if (foilmsg_encrypt(out, &body, NULL, &headers,
            iPrivateKey, iPublicKey, Util::encryptionOptions(&opt), NULL)) {
            iNewFile = QString::fromLocal8Bit(dest->str, dest->len);
            removeFile(iRemoveFile);
            foil_output_unref(out);
            if (chmod(dest->str, ENCRYPT_FILE_MODE) < 0) {
                HWARN("Failed to chmod" << dest->str << strerror(errno));
            }
        } else {
            foil_output_unref(out);
            unlink(dest->str);
        }

        if ((iTime || iToken.type() != FoilAuth::AuthTypeTOTP) && !isCanceled()) {
            iCurrentPassword = iToken.passwordString(iTime);
            if (iToken.type() == FoilAuth::AuthTypeTOTP) {
                iPrevPassword = iToken.passwordString(iTime - FoilAuth::PERIOD);
                iNextPassword = iToken.passwordString(iTime + FoilAuth::PERIOD);
            } else {
                iPrevPassword = iNextPassword = iCurrentPassword;
            }
            HDEBUG(qPrintable(iPrevPassword) << qPrintable(iCurrentPassword) <<
                qPrintable(iNextPassword));
        }
    }
    g_string_free(dest, TRUE);
}

// ==========================================================================
// FoilAuthModel::PasswordTask
// ==========================================================================

class FoilAuthModel::PasswordTask :
    public HarbourTask
{
    Q_OBJECT

public:
    PasswordTask(QThreadPool*, const ModelData*, quint64);

    void performTask() Q_DECL_OVERRIDE;

public:
    const QString iId;
    const FoilAuthToken iToken;
    const quint64 iTime;
    QString iPrevPassword;
    QString iCurrentPassword;
    QString iNextPassword;
};

FoilAuthModel::PasswordTask::PasswordTask(
    QThreadPool* aPool,
    const ModelData* aData,
    quint64 aTime) :
    HarbourTask(aPool),
    iId(aData->iId),
    iToken(aData->iToken),
    iTime(aTime)
{
    HDEBUG("Updating" << iToken.label());
}

void
FoilAuthModel::PasswordTask::performTask()
{
    iCurrentPassword = iToken.passwordString(iTime);
    iPrevPassword = iToken.passwordString(iTime - FoilAuth::PERIOD);
    iNextPassword = iToken.passwordString(iTime + FoilAuth::PERIOD);
    HDEBUG(iToken.label() << qPrintable(iPrevPassword) <<
        qPrintable(iCurrentPassword) << qPrintable(iNextPassword));
}

// ==========================================================================
// FoilNotesModel::SaveInfoTask
// ==========================================================================

class FoilAuthModel::SaveInfoTask :
    public BaseTask
{
    Q_OBJECT

public:
    SaveInfoTask(QThreadPool*, ModelInfo, const QString, FoilPrivateKey*,
        FoilKey*);

    void performTask() Q_DECL_OVERRIDE;

public:
    ModelInfo iInfo;
    QString iFoilDir;
};

FoilAuthModel::SaveInfoTask::SaveInfoTask(
    QThreadPool* aPool,
    ModelInfo aInfo,
    QString aFoilDir,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey) :
    BaseTask(aPool, aPrivateKey, aPublicKey),
    iInfo(aInfo),
    iFoilDir(aFoilDir)
{
}

void
FoilAuthModel::SaveInfoTask::performTask()
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

class FoilAuthModel::DecryptAllTask :
    public BaseTask
{
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

    DecryptAllTask(QThreadPool*, const QString, FoilPrivateKey*, FoilKey*);

    void performTask() Q_DECL_OVERRIDE;

    bool decryptToken(const QString aPath);

Q_SIGNALS:
    void progress(DecryptAllTask::Progress::Ptr);

public:
    const QString iDir;
    bool iSaveInfo;
    quint64 iTaskTime;
};

Q_DECLARE_METATYPE(FoilAuthModel::DecryptAllTask::Progress::Ptr)

FoilAuthModel::DecryptAllTask::DecryptAllTask(
    QThreadPool* aPool,
    const QString aDir,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey) :
    BaseTask(aPool, aPrivateKey, aPublicKey),
    iDir(aDir),
    iSaveInfo(false),
    iTaskTime(0)
{
}

bool
FoilAuthModel::DecryptAllTask::decryptToken(
    const QString aPath)
{
    bool ok = false;
    FoilMsg* aMsg = decryptAndVerify(aPath);
    if (aMsg) {
        QByteArray bytes(FoilAuth::toByteArray(aMsg->data));
        if (bytes.length() > 0) {
            ModelData* data = new ModelData(aPath,
                ModelData::headerAuthType(aMsg), bytes,
                ModelData::headerString(aMsg, HEADER_LABEL),
                ModelData::headerString(aMsg, HEADER_ISSUER),
                ModelData::headerInt(aMsg, HEADER_DIGITS, DEFAULT_DIGITS),
                ModelData::headerUint64(aMsg, HEADER_COUNTER, DEFAULT_COUNTER),
                ModelData::headerInt(aMsg, HEADER_TIMESHIFT, DEFAULT_TIMESHIFT),
                ModelData::headerAlgorithm(aMsg),
                ModelData::headerBool(aMsg, HEADER_FAVORITE, false));

            // Calculate current passwords while we are on it
            data->iCurrentPassword = data->iToken.passwordString(iTaskTime);
            data->iPrevPassword = data->iToken.passwordString(iTaskTime - FoilAuth::PERIOD);
            data->iNextPassword = data->iToken.passwordString(iTaskTime + FoilAuth::PERIOD);

            HDEBUG("Loaded secret from" << qPrintable(aPath));
            // The Progress takes ownership of ModelData
            Q_EMIT progress(Progress::Ptr(new Progress(data, this)));
            ok = true;
        }
        foilmsg_free(aMsg);
    }
    return ok;
}

void
FoilAuthModel::DecryptAllTask::performTask()
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

class FoilAuthModel::Private :
    public QObject
{
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
    ModelData* findData(const QString aId) const;
    QList<FoilAuthToken> getTokens(const QList<int> aRows) const;
    int findDataPos(const QString aId) const;
    bool needTimer() const;
    void queueSignal(Signal aSignal);
    void emitQueuedSignals();
    void updateTimer();
    void checkTimer();
    bool checkPassword(const QString);
    bool changePassword(const QString, const QString);
    void setKeys(FoilPrivateKey*, FoilKey* aPublic = Q_NULLPTR);
    void setFoilState(FoilState);
    void addToken(const FoilAuthToken&, bool aFavorite = true);
    void addTokens(const QList<FoilAuthToken>&);
    void insertModelData(ModelData*);
    void dataChanged(int , ModelData::Role);
    void dataChanged(QList<int>, ModelData::Role);
    void destroyItemAt(int);
    bool destroyItemAndRemoveFilesAt(int);
    bool destroyLastItemAndRemoveFiles();
    void deleteToken(QString);
    void deleteTokens(QStringList);
    void deleteAll();
    void clearModel();
    bool busy() const;
    void encrypt(const ModelData*);
    void updatePasswords(const ModelData*);
    void saveInfo();
    void generate(int, const QString);
    void lock(bool);
    bool unlock(const QString aPassword);
    bool encrypt(const QString aBody, const QColor aColor, uint aPageNr, uint aReqId);

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
    qint64 iLastPeriod;
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
    iLastPeriod(0),
    iTimeLeft(0)
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
    updateTimer();

    // Clear queued signals
    iFirstQueuedSignal = SignalCount;
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

inline FoilAuthModel*
FoilAuthModel::Private::parentModel()
{
    return qobject_cast<FoilAuthModel*>(parent());
}

void
FoilAuthModel::Private::queueSignal(
    Signal aSignal)
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

void
FoilAuthModel::Private::emitQueuedSignals()
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

inline
int
FoilAuthModel::Private::rowCount() const
{
    return iData.count();
}

inline
FoilAuthModel::ModelData*
FoilAuthModel::Private::dataAt(
    int aIndex) const
{
    if (aIndex >= 0 && aIndex < iData.count()) {
        return iData.at(aIndex);
    } else {
        return NULL;
    }
}

FoilAuthModel::ModelData*
FoilAuthModel::Private::findData(
    const QString aId) const
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

int
FoilAuthModel::Private::findDataPos(
    const QString aId) const
{
    const int n = iData.count();
    for (int i = 0; i < n; i++) {
        if (iData.at(i)->iId == aId) {
            return i;
        }
    }
    return -1;
}

QList<FoilAuthToken>
FoilAuthModel::Private::getTokens(
    const QList<int> aRows) const
{
    QList<FoilAuthToken> tokens;
    const int n = aRows.count();
    tokens.reserve(n);
    for (int i = 0; i < n; i++) {
        ModelData* data = dataAt(aRows.at(i));
        if (data) {
            tokens.append(data->iToken);
        }
    }
    return tokens;
}

void
FoilAuthModel::Private::setKeys(
    FoilPrivateKey* aPrivate,
    FoilKey* aPublic)
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

bool
FoilAuthModel::Private::checkPassword(
    const QString aPassword)
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

bool
FoilAuthModel::Private::changePassword(
    const QString aOldPassword,
    const QString aNewPassword)
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

bool
FoilAuthModel::Private::needTimer() const
{
    if (iFoilState == FoilModelReady) {
        const int n = iData.count();
        for (int i = 0; i < n; i++) {
            if (iData.at(i)->iToken.type() == FoilAuthTypes::AuthTypeTOTP) {
                return true;
            }
        }
    }
    return false;
}

void
FoilAuthModel::Private::checkTimer()
{
    if (needTimer()) {
        updateTimer();
    } else if (iTimer->isActive()){
        queueSignal(SignalTimerActiveChanged);
        iTimer->stop();
    }
}

void
FoilAuthModel::Private::setFoilState(FoilState aState)
{
    if (iFoilState != aState) {
        iFoilState = aState;
        HDEBUG(aState);
        checkTimer();
        queueSignal(SignalFoilStateChanged);
    }
}

void
FoilAuthModel::Private::dataChanged(
    int aIndex,
    ModelData::Role aRole)
{
    if (aIndex >= 0 && aIndex < iData.count()) {
        QVector<int> roles;
        roles.append(aRole);
        FoilAuthModel* model = parentModel();
        QModelIndex modelIndex(model->index(aIndex));
        Q_EMIT model->dataChanged(modelIndex, modelIndex, roles);
    }
}

void
FoilAuthModel::Private::dataChanged(
    QList<int> aRows,
    ModelData::Role aRole)
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

void
FoilAuthModel::Private::insertModelData(
    ModelData* aData)
{
    // Insert it into the model
    const int pos = iData.count();
    FoilAuthModel* model = parentModel();
    model->beginInsertRows(QModelIndex(), pos, pos);
    iData.append(aData);
    HDEBUG(aData->iId << aData->iToken.secretBase32() << aData->label());
    queueSignal(SignalCountChanged);
    checkTimer();
    model->endInsertRows();
}

void
FoilAuthModel::Private::onDecryptAllProgress(
    DecryptAllTask::Progress::Ptr aProgress)
{
    if (aProgress && aProgress->iTask == iDecryptAllTask) {
        // Transfer ownership of this ModelData to the model
        insertModelData(aProgress->iModelData);
        aProgress->iModelData = NULL;
    }
    emitQueuedSignals();
}

void
FoilAuthModel::Private::onDecryptAllTaskDone()
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

void
FoilAuthModel::Private::addToken(
    const FoilAuthToken& aToken,
    bool aFavorite)
{
    const QString path(FoilAuth::createEmptyFoilFile(iFoilDataDir));
    ModelData* data = new ModelData(path, aToken, aFavorite);
    insertModelData(data);
    updatePasswords(data);
    encrypt(data);
}

void
FoilAuthModel::Private::addTokens(
    const QList<FoilAuthToken>& aTokens)
{
    const int n = aTokens.size();
    if (n == 1) {
        addToken(aTokens.at(0), false);
    } else if (n > 1) {
        ModelData::List newData;
        int i;

        for (i = 0; i < n; i++) {
            FoilAuthToken token(aTokens.at(i));

            if (token.isValid()) {
                const QString path(FoilAuth::createEmptyFoilFile(iFoilDataDir));
                ModelData* data = new ModelData(path, token, false);

                // Password calculation and encryption happen asynchronously,
                // we can start doing it even before the data is inserted into
                // the model
                HDEBUG(data->iId << token.secretBase32() << token.label());
                updatePasswords(data);
                encrypt(data);
                newData.append(data);
            }
        }

        if (!newData.isEmpty()) {
            FoilAuthModel* model = parentModel();
            const int pos = iData.count();

            model->beginInsertRows(QModelIndex(), pos, pos + newData.count() - 1);
            iData.append(newData);
            queueSignal(SignalCountChanged);
            checkTimer();
            model->endInsertRows();
        }
    }
}

void
FoilAuthModel::Private::encrypt(
    const ModelData* aData)
{
    const bool wasBusy = busy();
    EncryptTask* task = new EncryptTask(iThreadPool, aData,
        iPrivateKey, iPublicKey, iLastPeriod * FoilAuth::PERIOD,
        iFoilDataDir);
    iEncryptTasks.append(task);
    task->submit(this, SLOT(onEncryptTaskDone()));
    if (!wasBusy) {
        // We must be busy now
        queueSignal(SignalBusyChanged);
    }
}

void
FoilAuthModel::Private::onEncryptTaskDone()
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

void
FoilAuthModel::Private::updatePasswords(
    const ModelData* aData)
{
    const bool wasBusy = busy();
    PasswordTask* task = new PasswordTask(iThreadPool, aData,
        iLastPeriod * FoilAuth::PERIOD);
    iPasswordTasks.append(task);
    task->submit(this, SLOT(onPasswordTaskDone()));
    if (!wasBusy) {
        // We must be busy now
        queueSignal(SignalBusyChanged);
    }
}

void
FoilAuthModel::Private::onPasswordTaskDone()
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

void
FoilAuthModel::Private::saveInfo()
{
    // N.B. This method may change the busy state but doesn't queue
    // BusyChanged signal, it's done by the caller.
    if (iSaveInfoTask) iSaveInfoTask->release();
    iSaveInfoTask = new SaveInfoTask(iThreadPool, ModelInfo(iData),
        iFoilDataDir, iPrivateKey, iPublicKey);
    iSaveInfoTask->submit(this, SLOT(onSaveInfoDone()));
}

void
FoilAuthModel::Private::onSaveInfoDone()
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

void
FoilAuthModel::Private::generate(
    int aBits,
    QString aPassword)
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

void
FoilAuthModel::Private::onGenerateKeyTaskDone()
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

void
FoilAuthModel::Private::destroyItemAt(
    int aIndex)
{
    if (aIndex >= 0 && aIndex <= iData.count()) {
        FoilAuthModel* model = parentModel();
        HDEBUG(iData.at(aIndex)->label());
        model->beginRemoveRows(QModelIndex(), aIndex, aIndex);
        delete iData.takeAt(aIndex);
        model->endRemoveRows();
        queueSignal(SignalCountChanged);
        checkTimer();
    }
}

bool
FoilAuthModel::Private::destroyItemAndRemoveFilesAt(
    int aIndex)
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

bool
FoilAuthModel::Private::destroyLastItemAndRemoveFiles()
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

void
FoilAuthModel::Private::deleteToken(
    QString aId)
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

void
FoilAuthModel::Private::deleteTokens(
    QStringList aIds)
{
    const bool wasBusy = busy();
    const int n = aIds.count();
    int deleted = 0;
    for (int i = 0; i < n; i++) {
        const QString id(aIds.at(i));
        if (destroyItemAndRemoveFilesAt(findDataPos(id))) {
            deleted++;
        } else {
            HDEBUG("Invalid token id" << id);
        }
    }
    if (deleted) {
        saveInfo();
        // saveInfo() doesn't queue BusyChanged signal, we have to do it here
        if (wasBusy != busy()) {
            queueSignal(SignalBusyChanged);
        }
    }
}

void
FoilAuthModel::Private::deleteAll()
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

void
FoilAuthModel::Private::clearModel()
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

void
FoilAuthModel::Private::lock(
    bool aTimeout)
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

bool
FoilAuthModel::Private::unlock(
    QString aPassword)
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

bool
FoilAuthModel::Private::busy() const
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

void
FoilAuthModel::Private::updateTimer()
{
    const qint64 msecsSinceEpoch = QDateTime::currentMSecsSinceEpoch();
    const qint64 secsSinceEpoch = msecsSinceEpoch / 1000;
    const qint64 thisPeriod = secsSinceEpoch / FoilAuth::PERIOD;
    const qint64 nextSecond = (secsSinceEpoch + 1) * 1000;
    const uint lastTimeLeft = iTimeLeft;

    if (!iTimer->isActive()) {
        queueSignal(SignalTimerActiveChanged);
    }
    iTimer->start((int)(nextSecond - msecsSinceEpoch));
    if (iLastPeriod != thisPeriod) {
        iLastPeriod = thisPeriod;
        iTimeLeft = FoilAuth::PERIOD;
        queueSignal(SignalTimeLeftChanged);
        const int n = iData.count();
        for (int i = 0; i < n; i++) {
            updatePasswords(iData.at(i));
        }
        Q_EMIT parentModel()->timerRestarted();
    } else {
        const qint64 endOfThisPeriod = (thisPeriod + 1) * FoilAuth::PERIOD;
        iTimeLeft = (int)(endOfThisPeriod - secsSinceEpoch);
    }

    if (lastTimeLeft != iTimeLeft) {
        queueSignal(SignalTimeLeftChanged);
    }
}

void
FoilAuthModel::Private::onTimer()
{
    updateTimer();
    emitQueuedSignals();
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
QObject*
FoilAuthModel::createSingleton(
    QQmlEngine*,
    QJSEngine*)
{
    return new FoilAuthModel;
}

int
FoilAuthModel::typeRole()
{
    return ModelData::TypeRole;
}

int
FoilAuthModel::favoriteRole()
{
    return ModelData::FavoriteRole;
}

Qt::ItemFlags
FoilAuthModel::flags(
    const QModelIndex& aIndex) const
{
    return SUPER::flags(aIndex) | Qt::ItemIsEditable;
}

QHash<int,QByteArray>
FoilAuthModel::roleNames() const
{
    QHash<int,QByteArray> roles;
#define ROLE(X,x) roles.insert(ModelData::X##Role, #x);
FOILAUTH_ROLES(ROLE)
#undef ROLE
    return roles;
}

int
FoilAuthModel::rowCount(
    const QModelIndex&) const
{
    return iPrivate->rowCount();
}

QVariant
FoilAuthModel::data(
    const QModelIndex& aIndex,
    int aRole) const
{
    ModelData* data = iPrivate->dataAt(aIndex.row());
    return data ? data->get((ModelData::Role)aRole) : QVariant();
}

bool
FoilAuthModel::setData(
    const QModelIndex& aIndex,
    const QVariant& aValue,
    int aRole)
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
                    data->iToken = data->iToken.withLabel(label);
                    iPrivate->encrypt(data);
                    iPrivate->emitQueuedSignals();
                    roles.append(aRole);
                    Q_EMIT dataChanged(aIndex, aIndex, roles);
                }
            }
            return true;
        case ModelData::SecretRole:
            {
                const QString base32(aValue.toString().toLower());
                const QByteArray secret(HarbourBase32::fromBase32(base32));
                HDEBUG(row << "secret" << base32 << "->" << secret.size() << "bytes");
                if (secret.size() > 0) {
                    if (data->iToken.secret() != secret) {
                        // Secret has actually changed
                        data->iToken = data->iToken.withSecret(secret);
                        iPrivate->encrypt(data);
                        iPrivate->emitQueuedSignals();
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
                    if (data->iToken.digits() != digits) {
                        data->iToken = data->iToken.withDigits(digits);
                        iPrivate->encrypt(data);
                        iPrivate->emitQueuedSignals();
                        roles.append(aRole);
                        Q_EMIT dataChanged(aIndex, aIndex, roles);
                    }
                    return true;
                }
            }
            break;
        case ModelData::TypeRole:
            {
                bool ok;
                const FoilAuthTypes::AuthType type =
                    (FoilAuthTypes::AuthType) aValue.toInt(&ok);
                if (ok && FoilAuthToken::validType(type) == type) {
                    HDEBUG(row << "type" << type);
                    if (data->iToken.type() != type) {
                        data->iToken = data->iToken.withType(type);
                        iPrivate->encrypt(data);
                        iPrivate->emitQueuedSignals();
                        roles.append(aRole);
                        Q_EMIT dataChanged(aIndex, aIndex, roles);
                    }
                    return true;
                }
            }
            break;
        case ModelData::AlgorithmRole:
            {
                bool ok;
                const FoilAuthTypes::DigestAlgorithm alg =
                    (FoilAuthTypes::DigestAlgorithm) aValue.toInt(&ok);
                if (ok) {
                    HDEBUG(row << "algorithm" << alg);
                    if (data->iToken.algorithm() != alg) {
                        data->iToken = data->iToken.withAlgorithm(alg);
                        iPrivate->encrypt(data);
                        iPrivate->emitQueuedSignals();
                        roles.append(aRole);
                        Q_EMIT dataChanged(aIndex, aIndex, roles);
                    }
                    return true;
                }
            }
            break;
        case ModelData::CounterRole:
            {
                bool ok;
                const quint64 counter = aValue.toULongLong(&ok);
                HDEBUG(row << "counter" << counter);
                if (ok) {
                    if (data->iToken.counter() != counter) {
                        data->iToken = data->iToken.withCounter(counter);
                        iPrivate->encrypt(data);
                        iPrivate->emitQueuedSignals();
                        roles.append(aRole);
                        Q_EMIT dataChanged(aIndex, aIndex, roles);
                    }
                    return true;
                }
            }
            break;
        case ModelData::TimeshiftRole:
            {
                bool ok;
                int sec = aValue.toInt(&ok);
                HDEBUG(row << "timeshift" << sec);
                if (ok) {
                    if (data->iToken.timeshift() != sec) {
                        data->iToken = data->iToken.withTimeshift(sec);
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

bool
FoilAuthModel::moveRows(
    const QModelIndex& aSrcParent,
    int aSrcRow,
    int,
    const QModelIndex& aDestParent,
    int aDestRow)
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

int
FoilAuthModel::period() const
{
    return FoilAuth::PERIOD;
}

int
FoilAuthModel::timeLeft() const
{
    return iPrivate->iTimeLeft;
}

bool
FoilAuthModel::busy() const
{
    return iPrivate->busy();
}

bool
FoilAuthModel::keyAvailable() const
{
    return iPrivate->iPrivateKey != Q_NULLPTR;
}

bool
FoilAuthModel::timerActive() const
{
    return iPrivate->iTimer->isActive();
}

FoilAuthModel::FoilState
FoilAuthModel::foilState() const
{
    return iPrivate->iFoilState;
}

bool
FoilAuthModel::checkPassword(
    QString aPassword)
{
    return iPrivate->checkPassword(aPassword);
}

bool
FoilAuthModel::changePassword(
    QString aOld,
    QString aNew)
{
    bool ok = iPrivate->changePassword(aOld, aNew);
    iPrivate->emitQueuedSignals();
    return ok;
}

void
FoilAuthModel::generateKey(
    int aBits,
    const QString aPassword)
{
    iPrivate->generate(aBits, aPassword);
    iPrivate->emitQueuedSignals();
}

void
FoilAuthModel::lock(
    bool aTimeout)
{
    iPrivate->lock(aTimeout);
    iPrivate->emitQueuedSignals();
}

bool
FoilAuthModel::unlock(
    const QString aPassword)
{
    const bool ok = iPrivate->unlock(aPassword);
    iPrivate->emitQueuedSignals();
    return ok;
}

bool
FoilAuthModel::addToken(
    int aType,
    QString aSecretBase32,
    QString aLabel,
    QString aIssuer,
    int aDigits,
    int aCounter,
    int aTimeShift,
    int aAlgorithm)
{
    HDEBUG(aSecretBase32 << aLabel << aIssuer << aDigits << aCounter << aTimeShift << aAlgorithm);
    const QByteArray secretBytes(HarbourBase32::fromBase32(aSecretBase32));
    if (secretBytes.size() > 0) {
        iPrivate->addToken(FoilAuthToken((FoilAuthTypes::AuthType) aType,
            secretBytes, aLabel, aIssuer, aDigits, aCounter, aTimeShift,
            (FoilAuthTypes::DigestAlgorithm) aAlgorithm));
        iPrivate->emitQueuedSignals();
        return true;
    }
    return false;
}

void
FoilAuthModel::addToken(
    FoilAuthToken aToken,
    bool aFavorite)
{
    if (aToken.isValid()) {
        HDEBUG(aToken.label());
        iPrivate->addToken(aToken, aFavorite);
        iPrivate->emitQueuedSignals();
    }
}

void
FoilAuthModel::addTokens(
    const QList<FoilAuthToken> aTokens)
{
    HDEBUG(aTokens);
    iPrivate->addTokens(aTokens);
}

void
FoilAuthModel::deleteToken(
    const QString aId)
{
    HDEBUG(aId);
    iPrivate->deleteToken(aId);
    iPrivate->emitQueuedSignals();
}

void
FoilAuthModel::deleteTokens(
    const QStringList aIds)
{
    HDEBUG(aIds);
    iPrivate->deleteTokens(aIds);
    iPrivate->emitQueuedSignals();
}

void
FoilAuthModel::deleteAll()
{
    HDEBUG("deleting all tokena");
    iPrivate->deleteAll();
    iPrivate->emitQueuedSignals();
}

QStringList
FoilAuthModel::getIdsAt(
    const QList<int> aRows) const
{
    QStringList ids;
    const int n = aRows.count();
    ids.reserve(n);
    for (int i = 0; i < n; i++) {
        ModelData* data = iPrivate->dataAt(aRows.at(i));
        if (data) {
            ids.append(data->iId);
        }
    }
    return ids;
}

int
FoilAuthModel::indexOf(
    FoilAuthToken aToken) const
{
    if (aToken.isValid()) {
        const int n = iPrivate->rowCount();
        for (int i = 0; i < n; i++) {
            if (iPrivate->dataAt(i)->iToken.equals(aToken)) {
                return i;
            }
        }
    }
    return -1;
}

bool
FoilAuthModel::containsSecret(
    const QByteArray aSecret) const
{
    const int n = iPrivate->rowCount();
    for (int i = 0; i < n; i++) {
        if (iPrivate->dataAt(i)->iToken.secret() == aSecret) {
            return true;
        }
    }
    return false;
}

QStringList
FoilAuthModel::generateMigrationUris(
    const QList<int> aRows) const
{
    HDEBUG(aRows);
    const QList<FoilAuthToken> tokens(iPrivate->getTokens(aRows));
    const QList<QByteArray> batch(FoilAuthToken::toProtoBufs(tokens));
    QStringList result;
    for (int k = 0; k < batch.size(); k++) {
        result.append(FoilAuth::migrationUri(batch.at(k)));
    }
    return result;
}

#include "FoilAuthModel.moc"
