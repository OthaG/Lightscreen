/*
 * Copyright (C) 2016  Christian Kaiser
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef UPLOADER_H
#define UPLOADER_H

#include <QObject>
#include <QtNetwork>
#include "imageuploader.h"

class Uploader : public QObject
{
    Q_OBJECT

public:
    Uploader(QObject *parent = 0);
    static Uploader *instance();
    QString lastUrl() const;
    int progress() const;
    QNetworkAccessManager *nam();
    static QString serviceName(int index);

public slots:
    void cancel();
    void upload(const QString &fileName, const QString &uploadService);
    int  uploading();
    void progressChanged(int p); //TODO: Rename

signals:
    void done(const QString &fileName, const QString &url, const QString &deleteHash);
    void error(const QString &errorString);
    void progress(int progress);
    void cancelAll();

private:
    static Uploader *mInstance;
    QNetworkAccessManager *mNetworkAccessManager;

    int mProgress;
    QString mLastUrl;
    QList<ImageUploader *> mUploaders;
};

#endif // UPLOADER_H
