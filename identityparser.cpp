#include "identityparser.h"

const QString IdentityParser::HEADER = "sqrldata";
const QString IdentityParser::HEADER_BASE64 = "SQRLDATA";

void IdentityParser::parseFile(QString fileName, IdentityModel* model)
{
    if (fileName.isEmpty() || !model)
    {
        throw std::invalid_argument("Both filename and model must be valid arguments!");
    }

    QFile identityFile(fileName);
    if (!identityFile.open(QIODevice::ReadOnly)) throw std::runtime_error("Error reading identity file!");
    QByteArray ba = identityFile.readAll();
    identityFile.close();

    parse(ba, model);
}

void IdentityParser::parseText(QByteArray identityText, IdentityModel* model)
{
    if (identityText.isEmpty() || !model)
    {
        throw std::invalid_argument("Both filename and model must be valid arguments!");
    }

    parse(identityText, model);
}

void IdentityParser::parse(QByteArray data, IdentityModel* model)
{
    if (!checkHeader(data))
    {
        throw std::runtime_error("Invalid header!");
    }

    if (m_bIsBase64)
    {
        QString decoded = QString::fromLocal8Bit(data);
        decoded = decoded.mid(HEADER.length());
        decoded = QString::fromLocal8Bit(
                    QByteArray::fromBase64(
                        decoded.toLocal8Bit(),
                        QByteArray::OmitTrailingEquals | QByteArray::Base64UrlEncoding));
        if (decoded.isEmpty()) throw std::runtime_error("Invalid base64-format on identity!");

        decoded = HEADER + decoded;
        data = decoded.toLocal8Bit();
    }

    int i = HEADER.length(); // skip header

    while (i < data.length())
    {
        uint16_t blockLength = getBlockLength(&(data.data()[i]));
        uint16_t blockType = getBlockType(&(data.data()[i]));

        bool isUnknownBlock = false;
        QByteArray sBlockDef;
        sBlockDef = getBlockDefinition(blockType);
        if (sBlockDef.isEmpty())
        {
            sBlockDef = getUnknownBlockDefinition();
            isUnknownBlock = true;
        }

        QJsonDocument blockDef = QJsonDocument::fromJson(sBlockDef, nullptr);
        IdentityModel::IdentityBlock block = parseBlock(&(data.data()[i]), &blockDef, isUnknownBlock);

        model->blocks.push_back(block);

        i += blockLength;
    }
}

IdentityModel::IdentityBlock IdentityParser::parseBlock(const char* data, QJsonDocument* blockDef, bool isUnknownBlock)
{
    IdentityModel::IdentityBlock newBlock;
    int index = 0;
    auto bd = (*blockDef);

    newBlock.blockType = bd["block_type"].toInt();
    newBlock.description = bd["description"].toString();
    newBlock.color = bd["color"].toString();

    QJsonArray items = bd["items"].toArray();

    for (int i=0; i<items.size(); i++)
    {
        IdentityModel::IdentityBlockItem newItem;
        QJsonObject item = items.at(i).toObject();

        int repeat_count = 1;
        if (item.contains("repeat_index"))
        {
            size_t repeat_index = static_cast<size_t>(item["repeat_index"].toInt());
            if (newBlock.items.size() > repeat_index)
            {
                repeat_count = newBlock.items[repeat_index].value.toInt();
            }
        }

        newItem.name = item["name"].toString();
        newItem.description = item["description"].toString();
        newItem.type = item["type"].toString();
        newItem.bytes = item["bytes"].toInt();

        for (int j=0; j<repeat_count; j++)
        {
            if (newItem.type == "UINT_8")
            {
                if (newItem.bytes != 1) throw std::runtime_error("Invalid byte count for datatype UINT_8!");
                newItem.value = parseUint8(data, index);
            }
            else if (newItem.type == "UINT_16")
            {
                if (newItem.bytes != 2) throw std::runtime_error("Invalid byte count for datatype UINT_16!");
                newItem.value = parseUint16(data, index);
            }
            else if (newItem.type == "UINT_32")
            {
                if (newItem.bytes != 4) throw std::runtime_error("Invalid byte count for datatype UINT_32!");
                newItem.value = parseUint32(data, index);
            }
            else if (newItem.type == "BYTE_ARRAY")
            {
                if (newItem.bytes < 1) throw std::runtime_error("Invalid byte count for datatype BYTE_ARRAY!");
                newItem.value = parseByteArray(data, index, newItem.bytes);
            }
            else
            {
                item["value"] = "";
            }

            newBlock.items.push_back(newItem);
            index += newItem.bytes;
        }
    }

    return newBlock;
}

bool IdentityParser::checkHeader(QByteArray data)
{
    QString header(data);
    header = header.left(HEADER.length());

    if (header != HEADER)
    {
        if (header == HEADER_BASE64)
        {
            this->m_bIsBase64 = true;
            return true;
        }
        else
        {
            return false;
        }
    }

    return true;
}

QByteArray IdentityParser::getBlockDefinition(uint16_t blockType)
{
    QDir path = QDir::currentPath();
    QDir fullPath = path.filePath(QString("blockdef/") + QString::number(blockType) + ".json");
    QString sFullPath = fullPath.absolutePath();

    QByteArray ba;
    QFile file(sFullPath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
    {
        return ba;
    }
    ba = file.readAll();
    file.close();

    return ba;    
}

QByteArray IdentityParser::getUnknownBlockDefinition()
{
    QByteArray ba;
    QString resFile = ":/res/file/unknown_blockdef.json";
    QFile file(resFile);
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
    {
        throw std::runtime_error("Error accessing resource file " + resFile.toStdString());
    }
    ba = file.readAll();
    file.close();

    return ba;
}


uint16_t IdentityParser::getBlockLength(const char* data)
{
    unsigned char c1 = static_cast<unsigned char>(data[0]);
    unsigned char c2 = static_cast<unsigned char>(data[1]);
    return static_cast<uint16_t>(c1 | (c2 << 8));
}

uint16_t IdentityParser::getBlockType(const char* data)
{
    unsigned char c1 = static_cast<unsigned char>(data[2]);
    unsigned char c2 = static_cast<unsigned char>(data[3]);
    return static_cast<uint16_t>(c1 | (c2 << 8));
}

QString IdentityParser::parseUint8(const char* data, int offset)
{
    return QString::number(static_cast<unsigned char>(data[offset]));
}

QString IdentityParser::parseUint16(const char* data, int offset)
{
    unsigned char c1 = static_cast<unsigned char>(data[offset]);
    unsigned char c2 = static_cast<unsigned char>(data[offset+1]);
    uint32_t num = static_cast<uint32_t>(c1 | (c2 << 8));
    return QString::number(num);
}

QString IdentityParser::parseUint32(const char* data, int offset)
{
    unsigned char c1 = static_cast<unsigned char>(data[offset]);
    unsigned char c2 = static_cast<unsigned char>(data[offset+1]);
    unsigned char c3 = static_cast<unsigned char>(data[offset+2]);
    unsigned char c4 = static_cast<unsigned char>(data[offset+3]);
    uint32_t num = static_cast<uint32_t>(c1 | (c2 << 8) | (c3 << 16) | (c4 << 24));
    return QString::number(num);
}

QString IdentityParser::parseByteArray(const char* data, int offset, int bytes)
{
    QByteArray ba(&data[offset], bytes);
    return QString::fromLocal8Bit(ba.toHex());
}
