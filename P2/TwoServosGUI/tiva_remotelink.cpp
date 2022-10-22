#include "tiva_remotelink.h"

//Implementacion de la clase que permite controlar el microcontrolador TIVA mediante el interfaz RPC


#include <QSerialPort>      // Comunicacion por el puerto serie
#include <QSerialPortInfo>  // Comunicacion por el puerto serie



TivaRemoteLink::TivaRemoteLink(QObject *parent) : QObject(parent)
{

    connected=false;

    // Las funciones CONNECT son la base del funcionamiento de QT; conectan dos componentes
    // o elementos del sistema; uno que GENERA UNA SEÑAL; y otro que EJECUTA UNA FUNCION (SLOT) al recibir dicha señal.
    // En el ejemplo se conecta la señal readyRead(), que envía el componente que controla el puerto USB serie (serial),
    // con la propia clase PanelGUI, para que ejecute su funcion readRequest() en respuesta.
    // De esa forma, en cuanto el puerto serie esté preparado para leer, se lanza una petición de datos por el
    // puerto serie.El envío de readyRead por parte de "serial" es automatico, sin necesidad de instrucciones
    // del programador
    connect(&serial, SIGNAL(readyRead()), this, SLOT(processIncommingSerialData()));
}


void TivaRemoteLink::sendMessage(uint8_t message_type, void *parameter, int32_t parameter_size)
{
    uint8_t frame[MAX_FRAME_SIZE];
    int32_t size;

    if (connected) // Para que no se intenten enviar datos si la conexion USB no esta activa
    {
            // El comando PING no necesita parametros; de ahí el NULL, y el 0 final.
            // No vamos a usar el mecanismo de numeracion de tramas; pasamos un 0 como n de trama
            size=create_frame((unsigned char *)frame, message_type, parameter, parameter_size, MAX_FRAME_SIZE);
            // Si la trama se creó correctamente, se escribe el paquete por el puerto serie USB
            if (size>0) serial.write((char *)frame,size);
    } else {
        LastError=QString("Status: conexion no establecida");
        emit statusChanged(TivaRemoteLink::TivaIsDisconnected,LastError);
    }
}

void TivaRemoteLink::sendMessage(uint8_t message_type, QByteArray parameter)
{
    sendMessage(message_type,parameter.data(),parameter.length());
}

//Este slot se conecta con la señal readyRead() del puerto serie, que se activa cuando hay algo que leer del puerto serie
//Se encarga de procesar y decodificar los datos que llegan de la TIVA y generar señales en respuesta a algunos de ellos
//Estas señales son capturadas por slots de la clase guipanel en este ejemplo.
void TivaRemoteLink::processIncommingSerialData()
{
    int StopCharPosition,StartCharPosition,tam;   // Solo uso notacin hungara en los elementos que se van a
    // intercambiar con el micro - para control de tamaño -
    uint8_t *pui8Frame; // Puntero a zona de memoria donde reside la trama recibida
    void *ptrtoparam;
    uint8_t ui8Command; // Para almacenar el comando de la trama entrante


    incommingDataBuffer.append(serial.readAll()); // Añade el contenido del puerto serie USB al array de bytes 'incommingDataBuffer'
    // así vamos acumulando  en el array la información que va llegando

    // Busca la posición del primer byte de fin de trama (0xFD) en el array. Si no estuviera presente,
    // salimos de la funcion, en caso contrario, es que ha llegado al menos una trama.
    // Hay que tener en cuenta que pueden haber llegado varios paquetes juntos.
    StopCharPosition=incommingDataBuffer.indexOf((char)STOP_FRAME_CHAR,0);
    while (StopCharPosition>=0)
    {
        //Ahora buscamos el caracter de inicio correspondiente.
        StartCharPosition=incommingDataBuffer.lastIndexOf((char)START_FRAME_CHAR,0); //Este seria el primer caracter de inicio que va delante...

        if (StartCharPosition<0)
        {
            //En caso de que no lo encuentre, no debo de hacer nada, pero debo vaciar las primeras posiciones hasta STOP_FRAME_CHAR (inclusive)
            incommingDataBuffer.remove(0,StopCharPosition+1);
            LastError=QString("Status:Fallo trozo paquete recibido");
            emit statusChanged(TivaRemoteLink::FragmentedPacketError,LastError);

        } else
        {
            incommingDataBuffer.remove(0,StartCharPosition); //Si hay datos anteriores al caracter de inicio, son un trozo de trama incompleto. Los tiro.
            tam=StopCharPosition-StartCharPosition+1;//El tamanio de la trama es el numero de bytes desde inicio hasta fin, ambos inclusive.
            if (tam>=MINIMUN_FRAME_SIZE)
            {
                pui8Frame=(uint8_t*)incommingDataBuffer.data(); // Puntero de trama al inicio del array de bytes
                pui8Frame++; //Nos saltamos el caracter de inicio.
                tam-=2; //Descontamos los bytes de inicio y fin del tamanio del paquete

                // Paso 1: Destuffing y cálculo del CRC. Si todo va bien, obtengo la trama
                // con valores actualizados y sin bytes de CRC.
                tam=destuff_and_check_checksum((unsigned char *)pui8Frame,tam);
                if (tam>=0)
                {
                    //El paquete está bien, luego procedo a tratarlo.
                    ui8Command=decode_command_type(pui8Frame); // Obtencion del byte de Comando
                    tam=get_command_param_pointer(pui8Frame,tam,&ptrtoparam);                    

                    emit messageReceived(ui8Command,QByteArray::fromRawData((char *)ptrtoparam,tam));
                }
                else
                {
                    LastError=QString("Status: Error de stuffing o CRC");
                    emit statusChanged(TivaRemoteLink::CRCorStuffError,LastError);
                    //Avisar con algo, no????
                }
            }
            else
            {

                // B. La trama no está completa o no tiene el tamano adecuado... no lo procesa
                //Este error lo notifico mediante la señal statusChanged
                LastError=QString("Status: Error trozo paquete recibido");
                emit statusChanged(TivaRemoteLink::FragmentedPacketError,LastError);
            }
            incommingDataBuffer.remove(0,StopCharPosition-StartCharPosition+1); //Elimino el trozo que ya he procesado
        }

        StopCharPosition=incommingDataBuffer.indexOf((char)STOP_FRAME_CHAR,0); //Compruebo si el se ha recibido alguna trama completa mas. (Para ver si tengo que salir del bucle o no
    } //Fin del while....
}

// Este método realiza el establecimiento de la comunicación USB serie con la TIVA a través del interfaz seleccionado
// Se establece una comunicacion a 9600bps 8N1 y sin control de flujo en el objeto
// 'serial' que es el que gestiona la comunicación USB serie en el interfaz QT
// Si la conexion no es correcta, se generan señales de error.
void TivaRemoteLink::startRemoteLink(QString puerto)
{
    if (serial.portName() != puerto) {
        serial.close();
        serial.setPortName(puerto);

        if (!serial.open(QIODevice::ReadWrite)) {
            LastError=QString("No puedo abrir el puerto %1, error code %2")
                         .arg(serial.portName()).arg(serial.error());

            emit statusChanged(TivaRemoteLink::OpenPortError,LastError);
            return ;
        }

        if (!serial.setBaudRate(9600)) {
            LastError=QString("No puedo establecer tasa de 9600bps en el puerto %1, error code %2")
                         .arg(serial.portName()).arg(serial.error());

            emit statusChanged(TivaRemoteLink::BaudRateError,LastError);
            return;
        }

        if (!serial.setDataBits(QSerialPort::Data8)) {
            LastError=QString("No puedo establecer 8bits de datos en el puerto %1, error code %2")
                         .arg(serial.portName()).arg(serial.error());


             emit statusChanged(TivaRemoteLink::DataBitError,LastError);
            return;
        }

        if (!serial.setParity(QSerialPort::NoParity)) {
            LastError=QString("NO puedo establecer parida en el puerto %1, error code %2")
                         .arg(serial.portName()).arg(serial.error());

            emit statusChanged(TivaRemoteLink::ParityError,LastError);
            return ;
        }

        if (!serial.setStopBits(QSerialPort::OneStop)) {
            LastError=QString("No puedo establecer 1bitStop en el puerto %1, error code %2")
                         .arg(serial.portName()).arg(serial.error());
            emit statusChanged(TivaRemoteLink::StopError,LastError);
            return;
        }

        if (!serial.setFlowControl(QSerialPort::NoFlowControl)) {
            LastError=QString("No puedo establecer el control de flujo en el puerto %1, error code %2")
                         .arg(serial.portName()).arg(serial.error());
             emit statusChanged(TivaRemoteLink::FlowControlError,LastError);
            return;
        }
    }

     emit statusChanged(TivaRemoteLink::TivaConnected,QString(""));

    // Variable indicadora de conexión a TRUE, para que se permita enviar comandos en respuesta
    // a eventos del interfaz gráfico
    connected=true;
}

//Método para leer el último mensaje de error
QString TivaRemoteLink::getLastErrorMessage()
{
    return LastError;
}



