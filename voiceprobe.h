#ifndef AUDIOPROBEDEVICE_H
#define AUDIOPROBEDEVICE_H

#include <QIODevice>

class AudioProbeDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit AudioProbeDevice(QObject* parent = nullptr) : QIODevice(parent) {}

    void start() { open(QIODevice::WriteOnly); }
    void stop() { close(); }

signals:
    void audioChunkReady(const QByteArray& data);

protected:
    qint64 readData(char*, qint64) override { return 0; }

    qint64 writeData(const char* data, qint64 len) override {
        if (len > 0) {
            emit audioChunkReady(QByteArray(data, len));
        }
        return len;
    }
};

#endif
