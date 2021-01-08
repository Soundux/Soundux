// By Dmitry Sazonov
#pragma once
#include <QCryptographicHash>
#include <QObject>
#include <QSharedMemory>
#include <QSystemSemaphore>

namespace Soundux
{
    namespace internal
    {
        QString generateKeyHash(const QString &key, const QString &salt)
        {
            QByteArray data;

            data.append(key.toUtf8());
            data.append(salt.toUtf8());
            data = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();

            return data;
        }
    } // namespace internal

    class RunGuard // NOLINT
    {
      public:
        RunGuard(const QString &key)
            : key(key), memLockKey(internal::generateKeyHash(key, "_memLockKey")),
              sharedmemKey(internal::generateKeyHash(key, "_sharedmemKey")), sharedMem(sharedmemKey),
              memLock(memLockKey, 1)
        {
            memLock.acquire();
            {
                QSharedMemory fix(sharedmemKey);
                fix.attach();
            }
            memLock.release();
        }
        ~RunGuard()
        {
            release();
        }

        bool isAnotherRunning()
        {
            if (sharedMem.isAttached())
                return false;

            memLock.acquire();
            const bool isRunning = sharedMem.attach();
            if (isRunning)
                sharedMem.detach();
            memLock.release();

            return isRunning;
        }
        bool tryToRun()
        {
            if (isAnotherRunning())
                return false;

            memLock.acquire();
            const bool result = sharedMem.create(sizeof(std::uint64_t));
            memLock.release();
            if (!result)
            {
                release();
                return false;
            }

            return true;
        }
        void release()
        {
            memLock.acquire();
            if (sharedMem.isAttached())
                sharedMem.detach();
            memLock.release();
        }

      private:
        const QString key;
        const QString memLockKey;
        const QString sharedmemKey;

        QSharedMemory sharedMem;
        QSystemSemaphore memLock;
    };
} // namespace Soundux