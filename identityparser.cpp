/*
 * This file is part of the "IdTool" utility app.
 *
 * MIT License
 *
 * Copyright (c) 2019 Alexander Hauser
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "identityparser.h"

const QString IdentityParser::HEADER = "sqrldata";
const QString IdentityParser::HEADER_BASE64 = "SQRLDATA";

void IdentityParser::parseFile(QString fileName, IdentityModel* model)
{
    if (fileName.isEmpty() || !model)
    {
        throw std::invalid_argument(
                    QObject::tr("Both filename and model must be valid arguments!")
                    .toStdString());
    }

    QFile identityFile(fileName);

    if (!identityFile.open(QIODevice::ReadOnly))
    {
        throw std::runtime_error(QObject::tr("Error reading identity file!")
                .toStdString());
    }

    QByteArray ba = identityFile.readAll();
    identityFile.close();

    parse(ba, model);
}

void IdentityParser::parseText(QByteArray identityText, IdentityModel* model)
{
    if (identityText.isEmpty() || !model)
    {
        throw std::invalid_argument(
                    QObject::tr("Both filename and model must be valid arguments!")
                    .toStdString());
    }

    parse(identityText, model);
}

void IdentityParser::parse(QByteArray data, IdentityModel* model)
{
    if (!checkHeader(data))
    {
        throw std::runtime_error(QObject::tr("Invalid header!")
                                 .toStdString());
    }

    if (m_bIsBase64)
    {
        QByteArray dataWithoutHeader = data.mid(HEADER.length());
        QByteArray decodedData = QByteArray::fromBase64(
                    dataWithoutHeader,
                    QByteArray::OmitTrailingEquals |
                    QByteArray::Base64UrlEncoding);

        if (decodedData.isEmpty())
        {
            throw std::runtime_error(
                        QObject::tr("Invalid base64-format on identity!")
                        .toStdString());
        }

        data = HEADER.toLocal8Bit() + decodedData;
    }

    int i = HEADER.length(); // skip header

    while (i < data.length())
    {
        uint16_t blockLength = getBlockLength(&(data.data()[i]));
        uint16_t blockType = getBlockType(&(data.data()[i]));

        QByteArray baBlockDef = getBlockDefinition(blockType);
        if (baBlockDef.isNull() || baBlockDef.isEmpty())
        {
            baBlockDef = getUnknownBlockDefinition();
        }

        QJsonParseError error;
        QJsonDocument blockDef = QJsonDocument::fromJson(baBlockDef, &error);

        if (error.error == QJsonParseError::NoError)
        {
            IdentityBlock block = parseBlock(&(data.data()[i]), &blockDef);
            model->blocks.push_back(block);
        }

        i += blockLength;
    }
}

IdentityBlock IdentityParser::parseBlock(const char* data, QJsonDocument* blockDef)
{
    IdentityBlock newBlock;
    int index = 0;
    auto bd = (*blockDef);

    newBlock.blockType = bd["block_type"].toInt();
    newBlock.description = bd["description"].toString();
    newBlock.color = bd["color"].toString();

    QJsonArray items = bd["items"].toArray();

    for (int i=0; i<items.size(); i++)
    {
        IdentityBlockItem newItem;
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
                if (newItem.bytes != 1) throw std::runtime_error(
                            QObject::tr("Invalid byte count for datatype UINT_8!")
                            .toStdString());
                newItem.value = parseUint8(data, index);
            }
            else if (newItem.type == "UINT_16")
            {
                if (newItem.bytes != 2) throw std::runtime_error(
                            QObject::tr("Invalid byte count for datatype UINT_16!")
                            .toStdString());
                newItem.value = parseUint16(data, index);
            }
            else if (newItem.type == "UINT_32")
            {
                if (newItem.bytes != 4) throw std::runtime_error(
                            QObject::tr("Invalid byte count for datatype UINT_32!")
                            .toStdString());
                newItem.value = parseUint32(data, index);
            }
            else if (newItem.type == "BYTE_ARRAY")
            {
                // If "bytes is set to -1, we shall use all the remaining bytes in the block
                if (newItem.bytes < 0)
                {
                    if (newBlock.items.size() > 0 &&
                            newBlock.items.at(0).name.toLower() == "length")
                    {
                        newItem.bytes = newBlock.items.at(0).value.toInt() - index;
                    }
                }
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
        throw std::runtime_error(
                    QObject::tr("Error accessing resource file for unknown block definition!")
                    .toStdString());
    }
    ba = file.readAll();
    file.close();

    return ba;
}

IdentityBlock IdentityParser::createEmptyBlock(uint16_t blockType)
{
    IdentityBlock result;

    QByteArray jsonData = getBlockDefinition(blockType);
    if (jsonData.isNull() || jsonData.isEmpty()) return result;

    QJsonParseError error;
    QJsonDocument blockDef = QJsonDocument::fromJson(jsonData, &error);

    if (error.error != QJsonParseError::NoError) return result;

    QJsonObject json = blockDef.object();

    if (json.isEmpty()) return result;

    result.blockType = json["block_type"].toInt(-1);
    result.description = json["description"].toString("");
    result.color = json["color"].toString("rgb(0,0,0)");

    QJsonArray jsonItems = json["items"].toArray();

    foreach (QJsonValue jsonItem, jsonItems)
    {
        QJsonObject jsonItemObj = jsonItem.toObject();

        IdentityBlockItem item;
        item.name = jsonItemObj["name"].toString("");
        item.description = jsonItemObj["description"].toString("");
        item.type = jsonItemObj["type"].toString("UINT_8");
        item.bytes = jsonItemObj["name"].toInt(1);
        item.value = "";

        result.items.push_back(item);
    }

    return result;
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
