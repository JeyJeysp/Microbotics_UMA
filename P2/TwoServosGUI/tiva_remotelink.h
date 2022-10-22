#ifndef QTIVARPC_H
#define QTIVARPC_H

#include <QObject>
#include <QSerialPort>      // Comunicacion por el puerto serie
#include <QSerialPortInfo>  // Comunicacion por el puerto serie


#include<stdint.h>      // Cabecera para usar tipos de enteros con tamaño
#include<stdbool.h>     // Cabecera para usar booleanos

extern "C" {
#include "serialprotocol.h"    // Cabecera de funciones de gestión de tramas; se indica que está en C, ya que QTs
// se integra en C++, y el C puede dar problemas si no se indica.
#include "remotelink_messages.h"
}

class TivaRemoteLink : public QObject
{

    Q_OBJECT
public:
    explicit TivaRemoteLink(QObject *parent = 0);

    //Define una serie de etiqueta para los errores y estados notificados por la señal statusChanged(...)
    enum TivaStatus {TivaConnected,
                     TivaDisconnected,
                     OpenPortError,
                     BaudRateError,
                     DataBitError,
                     ParityError,
                     StopError,
                     FlowControlError,
                     UnexpectedPacketError,
                     FragmentedPacketError,
                     CRCorStuffError,
                     ReceivedDataError,
                     TivaIsDisconnected,
                    };
    Q_ENUM(TivaStatus)

    //Metodo publicos
    QString getLastErrorMessage();
    int32_t readMessageParameter(void *parameters,int32_t max_parameter_size);


signals:
    void statusChanged(int status, QString message); //Esta señal se genera al realizar la conexión/desconexion o cuando se produce un error de comunicacion
    void messageReceived(uint8_t message_type, QByteArray parameter);

public slots:
    void startRemoteLink(QString puerto); //Este Slot arranca la comunicacion
    void sendMessage(uint8_t message_type, void *parameter, int32_t parameter_size);
    void sendMessage(uint8_t message_type, QByteArray parameter);

private slots:
    void processIncommingSerialData(); //Este Slot se conecta a la señal readyRead(..) del puerto serie. Se encarga de procesar y decodificar los mensajes que llegan de la TIVA y
                        //generar señales para algunos de ellos
private:
    QSerialPort serial;
    QString LastError;
    bool connected;
    QByteArray incommingDataBuffer;

};

#endif // QTIVARPC_H
