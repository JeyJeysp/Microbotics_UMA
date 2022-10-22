#include "TwoServosGUI.h"
#include "ui_TwoServosGUI.h"
#include <QSerialPort>      // Comunicacion por el puerto serie
#include <QSerialPortInfo>  // Comunicacion por el puerto serie
#include <QMessageBox>      // Se deben incluir cabeceras a los componentes que se vayan a crear en la clase
// y que no estén incluidos en el interfaz gráfico. En este caso, la ventana de PopUp <QMessageBox>
// que se muestra al recibir un PING de respuesta

#include<stdint.h>      // Cabecera para usar tipos de enteros con tamaño
#include<stdbool.h>     // Cabecera para usar booleanos

TwoServosGUI::TwoServosGUI(QWidget *parent) :  // Constructor de la clase
    QWidget(parent),
    ui(new Ui::TwoServosGUI)               // Indica que TwoServosGUI.ui es el interfaz grafico de la clase
  , transactionCount(0)
{
    ui->setupUi(this);                // Conecta la clase con su interfaz gráfico.
    setWindowTitle(tr("Interfaz de Control de Servos")); // Título de la ventana

    // Inicializa la lista de puertos serie
    ui->serialPortComboBox->clear(); // Vacía de componentes la comboBox
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        // La descripción nos permite que SOLO aparezcan los interfaces tipo USB serial con el identificador de fabricante y producto de la TIVA
        if ((info.vendorIdentifier()==0x1CBE) && (info.productIdentifier()==0x0002))
        {
            ui->serialPortComboBox->addItem(info.portName());
        }
    }

    ui->serialPortComboBox->setFocus();   // Componente del GUI seleccionado de inicio
    ui->pingButton->setEnabled(false);    // Se deshabilita el botón de ping del interfaz gráfico, hasta que

    //Inicializa la ventana de PopUp que se muestra cuando llega la respuesta al PING
    ventanaPopUp.setIcon(QMessageBox::Information);
    ventanaPopUp.setText(tr("Status: RESPUESTA A PING RECIBIDA")); //Este es el texto que muestra la ventana
    ventanaPopUp.setStandardButtons(QMessageBox::Ok);
    ventanaPopUp.setWindowTitle(tr("Evento"));
    ventanaPopUp.setParent(this,Qt::Popup);

    //Conectamos Slots del objeto "Tiva" con Slots de nuestra aplicacion (o con widgets)
    connect(&tiva,SIGNAL(statusChanged(int,QString)),this,SLOT(tivaStatusChanged(int,QString)));
    connect(&tiva,SIGNAL(messageReceived(uint8_t,QByteArray)),this,SLOT(messageReceived(uint8_t,QByteArray)));

}

TwoServosGUI::~TwoServosGUI() // Destructor de la clase
{
    delete ui;   // Borra el interfaz gráfico asociado a la clase
}

void TwoServosGUI::on_serialPortComboBox_currentIndexChanged(const QString &arg1)
{
    activateRunButton();
}

// Funcion auxiliar de procesamiento de errores de comunicación
void TwoServosGUI::processError(const QString &s)
{
    activateRunButton(); // Activa el botón RUN
    // Muestra en la etiqueta de estado la razón del error (notese la forma de pasar argumentos a la cadena de texto)
    ui->statusLabel->setText(tr("Status: Not running, %1.").arg(s));
}

// Funcion de habilitacion del boton de inicio/conexion
void TwoServosGUI::activateRunButton()
{
    ui->runButton->setEnabled(true);
}

// SLOT asociada a pulsación del botón RUN
void TwoServosGUI::on_runButton_clicked()
{
    //Intenta arrancar la comunicacion con la TIVA
    //Intenta arrancar la comunicacion con la TIVA
    tiva.startRemoteLink( ui->serialPortComboBox->currentText());
}

//Slots Asociado al boton que limpia los mensajes del interfaz
void TwoServosGUI::on_pushButton_clicked()
{
    ui->statusLabel->setText(tr(""));
}

//**** Slots asociados a la recepción de mensajes desde la TIVA ********/
//Están conectados a señales generadas por el objeto TIVA de clase QRemoteTiva (se conectan en el constructor de la ventana, más arriba en este fichero))
//Se pueden añadir los que se quieran para completar la aplicación

//Este Slot lo hemos conectado con la señal statusChange de la TIVA, que se activa cuando
//El puerto se conecta o se desconecta y cuando se producen determinados errores.
//Esta función lo que hace es procesar dichos eventos
void TwoServosGUI::tivaStatusChanged(int status,QString message)
{
    switch (status)
    {
        case TivaRemoteLink::TivaConnected:

            //Caso conectado..
            // Deshabilito el boton de conectar
            ui->runButton->setEnabled(false);

            // Se indica que se ha realizado la conexión en la etiqueta 'statusLabel'
            ui->statusLabel->setText(tr("Ejecucion, conectado al puerto %1.")
                             .arg(ui->serialPortComboBox->currentText()));

            //    // Y se habilitan los controles deshabilitados
            ui->pingButton->setEnabled(true);
            ui->servo1->setEnabled(true);
            ui->servo2->setEnabled(true);
            ui->stopButton->setEnabled(true);
        break;

        case TivaRemoteLink::TivaIsDisconnected:
            //Caso desconectado..
            // Rehabilito el boton de conectar
            ui->runButton->setEnabled(false);
            ui->statusLabel->setText(message);
        break;
        case TivaRemoteLink::FragmentedPacketError:
        case TivaRemoteLink::CRCorStuffError:
            //Errores detectados en la recepcion de paquetes
            ui->statusLabel->setText(message);
        default:
            //Otros errores (por ejemplo, abriendo el puerto)
            processError(tiva.getLastErrorMessage());
    }
}


void TwoServosGUI::messageReceived(uint8_t message_type, QByteArray datos)
{
    switch(message_type) // Segun el comando tengo que hacer cosas distintas
    {
        /** A PARTIR AQUI ES DONDE SE DEBEN AÑADIR NUEVAS RESPUESTAS ANTE LOS COMANDOS QUE SE ENVIEN DESDE LA TIVA **/
        case MESSAGE_PING:
        {   //Recepcion de la respuesta al ping desde la TIVA
            // Algunos comandos no tiene parametros --> No se extraen
            ventanaPopUp.setStyleSheet("background-color: lightgrey");
            ventanaPopUp.setModal(true); //CAMBIO: Se sustituye la llamada a exec(...) por estas dos.
            ventanaPopUp.show();
        }
        break;

        case MESSAGE_REJECTED:
        {   //Recepcion del mensaje MESSAGE_REJECTED (La tiva ha rechazado un mensaje que le enviamos previamente)
            MESSAGE_REJECTED_PARAMETER parametro;
            if (check_and_extract_command_param(datos.data(), datos.size(), &parametro, sizeof(parametro))>0)
            {
                // Muestra en una etiqueta (statuslabel) del GUI el mensaje
            ui->statusLabel->setText(tr("Status: Comando rechazado,%1").arg(parametro.message));
            }
            else
            {
                ui->statusLabel->setText(tr("Status: MSG %1, recibí %2 B y esperaba %3").arg(message_type).arg(datos.size()).arg(sizeof(parametro)));
            }
        }
        break;
        default:
            //Notifico que ha llegado un tipo de mensaje desconocido
            ui->statusLabel->setText(tr("Status: Recibido mensaje desconocido %1").arg(message_type));
        break; //Innecesario
    }
}

void TwoServosGUI::on_pingButton_clicked()
{
    tiva.sendMessage(MESSAGE_PING,NULL,0);
}

void TwoServosGUI::on_servo1_sliderReleased()
{
    PARAM_MESSAGE_MOTOR parameter;
    parameter.id_motor = MOTOR_1;
    parameter.porcentaje_vel = (int8_t)ui->servo1->value();

    tiva.sendMessage(MESSAGE_MOTOR,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));
}


void TwoServosGUI::on_servo2_sliderReleased()
{
    PARAM_MESSAGE_MOTOR parameter;
    parameter.id_motor = MOTOR_2;
    parameter.porcentaje_vel = (int8_t)ui->servo2->value();

    tiva.sendMessage(MESSAGE_MOTOR,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));
}

void TwoServosGUI::on_stopButton_pressed()
{
    PARAM_MESSAGE_MOTOR parameter;
    parameter.id_motor = MOTORES;
    parameter.porcentaje_vel = 0;
    tiva.sendMessage(MESSAGE_MOTOR,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));
}
