#ifndef TWOSERVOSGUI_H
#define TWOSERVOSGUI_H

#include <QWidget>
#include <QtSerialPort/qserialport.h>
#include <QMessageBox>
#include "tiva_remotelink.h"

namespace Ui {
class TwoServosGUI;
}

//QT4:QT_USE_NAMESPACE_SERIALPORT

class TwoServosGUI : public QWidget
{
    Q_OBJECT
    
public:
    //TwoServosGUI(QWidget *parent = 0);
    explicit TwoServosGUI(QWidget *parent = 0);
    ~TwoServosGUI(); // Da problemas
    
private slots:
    // slots privados asociados mediante "connect" en el constructor
    void tivaStatusChanged(int status,QString message);
    void messageReceived(uint8_t type, QByteArray datos);

    //Slots asociados por nombre
    void on_runButton_clicked();
    void on_serialPortComboBox_currentIndexChanged(const QString &arg1);
    void on_pushButton_clicked();
    void on_pingButton_clicked();

    void on_servo1_sliderReleased();
    void on_servo2_sliderReleased();
    void on_stopButton_pressed();

private: // funciones privadas
    // funciones privadas
    void processError(const QString &s);
    void activateRunButton();
private:
    Ui::TwoServosGUI *ui;
    int transactionCount;
    QMessageBox ventanaPopUp;
    TivaRemoteLink tiva; //Objeto para gestionar la comunicacion de mensajes con el microcontrolador
};

#endif // TwoServosGUI_H
