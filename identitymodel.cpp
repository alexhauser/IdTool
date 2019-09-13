#include "identitymodel.h"
#include "identityparser.h"

void IdentityModel::writeToFile(QString fileName)
{
    QByteArray ba(IdentityParser::HEADER.toUtf8());

    for (size_t i=0; i<blocks.size(); i++)
    {
        ba.append(blocks[i].toByteArray());
    }

    QFile file(fileName);

    if (!file.open(QFile::WriteOnly))
    {
        throw std::runtime_error(
                QObject::tr("Error opening identity file for writing!")
                    .toStdString());
    }

    file.write(ba);
    file.close();
}

QByteArray IdentityBlock::toByteArray()
{
    QByteArray ba;

    for (size_t i=0; i<items.size(); i++)
    {
        if (items[i].type == "UINT_8")
        {
            ba.append(static_cast<char>(items[i].value.toInt()));
        }
        else if (items[i].type == "UINT_16")
        {
            int value = items[i].value.toInt();
            char* data = static_cast<char*>(static_cast<void*>(&value));
            ba.append(data[0]);
            ba.append(data[1]);
        }
        else if (items[i].type == "UINT_32")
        {
            QString v = items[i].value;
            int vi = v.toInt();
            uint32_t value = static_cast<uint32_t>(vi);
            char* data = static_cast<char*>(static_cast<void*>(&value));
            ba.append(data[0]);
            ba.append(data[1]);
            ba.append(data[2]);
            ba.append(data[3]);
        }
        else if (items[i].type == "BYTE_ARRAY")
        {
            QByteArray baTemp = QByteArray::fromHex(items[i].value.toUtf8());
            ba.append(baTemp);
        }
    }

    return ba;
}

void IdentityModel::deleteBlock(IdentityBlock* block)
{
    for (size_t i=0; i<blocks.size(); i++)
    {
        if (&blocks[i] == block)
        {
            blocks.erase(blocks.begin()+ static_cast<long long>(i));
            return;
        }
    }
}

bool IdentityModel::moveBlock(IdentityBlock* block, bool up)
{
    if (blocks.size() < 2) return false;

    for (size_t i=0; i<blocks.size(); i++)
    {
        if (&blocks[i] == block)
        {
            long long swapWith;

            if (up)
            {
                if (i == 0) return false;
                swapWith = static_cast<long long>(i) - 1;
            }
            else
            {
                if (i == (blocks.size() - 1)) return false;
                swapWith = static_cast<long long>(i) + 1;
            }

            iter_swap(blocks.begin() + static_cast<long long>(i), blocks.begin() + swapWith);
            return true;
        }
    }

    return false;
}
