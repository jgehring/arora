/*
 * Copyright 2009 Jonas Gehring <jonas.gehring@boolsoft.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "schemeaccesshandler.h"

#include <qapplication.h>
#include <qdatetime.h>
#include <qdir.h>
#include <qfileiconprovider.h>
#include <qstyle.h>
#include <qtextstream.h>
#include <qtimer.h>
#include <qwebsettings.h>

SchemeAccessHandler::SchemeAccessHandler(QObject *parent)
    : QObject(parent)
{
}


FileAccessHandler::FileAccessHandler(QObject *parent)
    : SchemeAccessHandler(parent)
{
}

QNetworkReply *FileAccessHandler::createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    Q_UNUSED(outgoingData);

    switch (op) {
    case QNetworkAccessManager::GetOperation:
        break;
    default:
        return 0;
    }

    // This handler can only list directories yet, so pass anything
    // else back to the manager
    QString path = request.url().toLocalFile();
    if (!QFileInfo(path).isDir()) {
        return 0;
    }

    FileAccessReply *reply = new FileAccessReply(request, this);
    return reply;
}


FileAccessReply::FileAccessReply(const QNetworkRequest &request, QObject *parent)
    : QNetworkReply(parent)
{
    setOperation(QNetworkAccessManager::GetOperation);
    setRequest(request);
    setUrl(request.url());

    buffer.open(QIODevice::ReadWrite);
    setError(QNetworkReply::NoError, tr("No Error"));

    QTimer::singleShot(0, this, SLOT(listDirectory()));

    open(QIODevice::ReadOnly);
}

FileAccessReply::~FileAccessReply()
{
    close();
}

qint64 FileAccessReply::bytesAvailable() const
{
    return buffer.bytesAvailable() + QNetworkReply::bytesAvailable();
}

void FileAccessReply::close()
{
    buffer.close();
}

static QString cssLinkClass(const QString &name, const QIcon &icon, int size = 32)
{
    QString data = QLatin1String("a.%1 {\n\
  padding-left: %2px;\n\
  background: transparent url(data:image/png;base64,%3) no-repeat center left;\n\
  font-weight: bold;\n\
}\n");
    QPixmap pixmap = icon.pixmap(QSize(size, size));
    QBuffer imageBuffer;
    imageBuffer.open(QBuffer::ReadWrite);
    if (!pixmap.save(&imageBuffer, "PNG")) {
        // If an error occured, write a blank pixmap
        pixmap = QPixmap(size, size);
        pixmap.fill(Qt::transparent);
        imageBuffer.buffer().clear();
        pixmap.save(&imageBuffer, "PNG");
    }
    return data.arg(name).arg(size+4).arg(QLatin1String(imageBuffer.buffer().toBase64()));
}

void FileAccessReply::listDirectory()
{
    QDir dir(url().toLocalFile());
    if (!dir.exists()) {
        setError(QNetworkReply::ContentNotFoundError, tr("Error opening: %1: No such file or directory").arg(dir.absolutePath()));
        emit error(QNetworkReply::ContentNotFoundError);
        emit finished();
        return;
    }
    if (!dir.isReadable()) {
        setError(QNetworkReply::ContentAccessDenied, tr("Unable to read %1").arg(dir.absolutePath()));
        emit error(QNetworkReply::ContentAccessDenied);
        emit finished();
        return;
    }

    // Format a html page for the directory contents
    QFile dirlistFile(QLatin1String(":/dirlist.html"));
    if (!dirlistFile.open(QIODevice::ReadOnly))
        return;
    QString html = QLatin1String(dirlistFile.readAll());
    html = html.arg(dir.absolutePath(), tr("Contents of %1").arg(dir.absolutePath()));

    QFileIconProvider iconProvider;
    QHash<qint64, bool> existingClasses;
    int iconSize = QWebSettings::globalSettings()->fontSize(QWebSettings::DefaultFontSize);
    QFileInfoList list = dir.entryInfoList(QDir::AllEntries | QDir::Hidden, QDir::Name | QDir::DirsFirst);
    QString dirlist, classes;
    for (int i = 0; i < list.count(); i++) {
        // Skip '.' and possibly '..'
        if (list[i].fileName() == QLatin1String(".") || (list[i].fileName() == QLatin1String("..") && dir.isRoot())) {
            continue;
        }

        // TODO: Why reconstruct the QFileInfo object?
        QString path = QFileInfo(dir.absoluteFilePath(list[i].fileName())).canonicalFilePath();
        QString addr = QString::fromUtf8(QUrl::fromLocalFile(path).toEncoded());
        QString link = QString(QLatin1String("<a class=\"%1\" href=\"%2\">%3</a>"));
        QIcon icon;
        if (list[i].fileName() == QLatin1String("..")) {
            icon = qApp->style()->standardIcon(QStyle::SP_FileDialogToParent);
        } else {
            icon = iconProvider.icon(list[i]);
        }
        QString className = QString(QLatin1String("link_%1")).arg(icon.cacheKey());
        if (!existingClasses.contains(icon.cacheKey())) {
            classes += cssLinkClass(className, icon, iconSize);
            existingClasses.insert(icon.cacheKey(), true);
        }

        link = link.arg(className).arg(addr).arg(list[i].fileName());

        QString size, modified;
        if (list[i].fileName() != QLatin1String("..")) {
            if (list[i].isFile())
                size = tr("%1 KB").arg(QString::number(list[i].size()/1024));
            modified = list[i].lastModified().toString(Qt::SystemLocaleShortDate);
        }

        dirlist += QString(QLatin1String("<tr> <td class=\"name\">%1</td> <td class=\"size\">%2</td> <td class=\"modified\">%3</td> </tr>\n")).arg(
                        link, size, modified);
    }

    html = html.arg(classes).arg(dirlist);

    // Save result to buffer
    QTextStream stream(&buffer);
    stream << html;
    stream.flush();
    buffer.reset();

    // Publish result
    setHeader(QNetworkRequest::ContentTypeHeader, QByteArray("text/html"));
    setHeader(QNetworkRequest::ContentLengthHeader, buffer.bytesAvailable());
    setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200);
    setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, QByteArray("Ok"));
    emit metaDataChanged();
    emit downloadProgress(buffer.size(), buffer.size());
    QNetworkReply::NetworkError errorCode = error();
    if (errorCode != QNetworkReply::NoError) {
        emit error(errorCode);
    } else if (buffer.size() > 0) {
        emit readyRead();
    }

    emit finished();
}

qint64 FileAccessReply::readData(char *data, qint64 maxSize)
{
    return buffer.read(data, maxSize);
}
