#include "mainusergui.h"
#include "ui_mainusergui.h"
#include <QSerialPort>      // Comunicacion por el puerto serie
#include <QSerialPortInfo>  // Comunicacion por el puerto serie
#include <QMessageBox>      // Se deben incluir cabeceras a los componentes que se vayan a crear en la clase
// y que no estén incluidos en el interfaz gráfico. En este caso, la ventana de PopUp <QMessageBox>
// que se muestra al recibir un PING de respuesta

#include<stdint.h>      // Cabecera para usar tipos de enteros con tamaño
#include<stdbool.h>     // Cabecera para usar booleanos

MainUserGUI::MainUserGUI(QWidget *parent) :  // Constructor de la clase
    QWidget(parent),
    ui(new Ui::MainUserGUI)               // Indica que guipanel.ui es el interfaz grafico de la clase
  , transactionCount(0)
{
    ui->setupUi(this);                // Conecta la clase con su interfaz gráfico.
    setWindowTitle(tr("Interfaz de Control TIVA")); // Título de la ventana

    // Inicializa la lista de puertos serie
    ui->serialPortComboBox->clear(); // Vacía de componentes la comboBox
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        // La descripción realiza un FILTRADO que  nos permite que SOLO aparezcan los interfaces tipo USB serial con el identificador de fabricante
        // y producto de la TIVA
        // NOTA!!: SI QUIERES REUTILIZAR ESTE CODIGO PARA OTRO CONVERSOR USB-Serie, cambia los valores VENDOR y PRODUCT por los correspondientes
        // o cambia la condicion por "if (true) para listar todos los puertos serie"
        if ((info.vendorIdentifier()==0x1CBE) && (info.productIdentifier()==0x0002))
        {
            ui->serialPortComboBox->addItem(info.portName());
        }
    }

    //Inicializa algunos controles
    ui->serialPortComboBox->setFocus();   // Componente del GUI seleccionado de inicio
    ui->pingButton->setEnabled(false);    // Se deshabilita el botón de ping del interfaz gráfico, hasta que

    //Inicializa la ventana de PopUp que se muestra cuando llega la respuesta al PING
    ventanaPopUp.setIcon(QMessageBox::Information);
    ventanaPopUp.setText(tr("Status: RESPUESTA A PING RECIBIDA")); //Este es el texto que muestra la ventana
    ventanaPopUp.setStandardButtons(QMessageBox::Ok);
    ventanaPopUp.setWindowTitle(tr("Evento"));
    ventanaPopUp.setParent(this,Qt::Popup);

    //Inicializo la grafica del Diferencial

    ui->GraficaDiff->setTitle("Grafica ADC DIferencial"); // Titulo de la grafica
    ui->GraficaDiff->setAxisTitle(QwtPlot::xBottom, "Tiempo"); // Etiqueta eje X de coordenadas
    ui->GraficaDiff->setAxisTitle(QwtPlot::yLeft, "Voltaje");    // Etiqueta eje Y de coordenadas
    //ui->Grafica->axisAutoScale(true); // Con Autoescala
    ui->GraficaDiff->setAxisScale(QwtPlot::yLeft, -3.3, 3.3); // Escala fija entre -3.3 y 3.3V
    ui->GraficaDiff->setAxisScale(QwtPlot::xBottom,0,1024.0);

    // Formateo de la curva, establezco numero de curvas y las asocio a la grafica
    for(int i=0;i<3;i++){
        ChannelsDiff[i] = new QwtPlotCurve();
        ChannelsDiff[i]->attach(ui->GraficaDiff);
    }

    //Inicializacion de los valores básicos, en este caso a 0??
    for(int i=0;i<1000;i++){
        xValDiff[i]=i;
        yValDiff[0][i]=0;
        yValDiff[1][i]=0;
        yValDiff[2][i]=0;
    }

    ChannelsDiff[0]->setRawSamples(xValDiff,yValDiff[0],1000);
    ChannelsDiff[1]->setRawSamples(xValDiff,yValDiff[1],1000);
    ChannelsDiff[2]->setRawSamples(xValDiff,yValDiff[2],1000);

    ChannelsDiff[0]->setPen(QPen(Qt::red)); // Color de la curva
    ChannelsDiff[1]->setPen(QPen(Qt::cyan));
    ChannelsDiff[2]->setPen(QPen(Qt::yellow));


    m_Grid_Diff = new QwtPlotGrid();        // Rejilla de puntos
    m_Grid_Diff->attach(ui->GraficaDiff);       // que se asocia al objeto QwtPl
    ui->GraficaDiff->setAutoReplot(false); //Desactiva el autoreplot (mejora la eficiencia)

    //termina inicializacion de la grafica

    //Inicializo la grafica

    ui->Grafica->setTitle("Grafica ADC"); // Titulo de la grafica
    ui->Grafica->setAxisTitle(QwtPlot::xBottom, "Tiempo"); // Etiqueta eje X de coordenadas
    ui->Grafica->setAxisTitle(QwtPlot::yLeft, "Voltaje");    // Etiqueta eje Y de coordenadas
    //ui->Grafica->axisAutoScale(true); // Con Autoescala
    ui->Grafica->setAxisScale(QwtPlot::yLeft, 0, 3.3); // Escala fija entre 0 y 3.3V
    ui->Grafica->setAxisScale(QwtPlot::xBottom,0,1024.0);

    // Formateo de la curva, establezco numero de curvas y las asocio a la grafica
    for(int i=0;i<6;i++){
        Channels[i] = new QwtPlotCurve();
        Channels[i]->attach(ui->Grafica);
    }

    //Inicializacion de los valores básicos, en este caso a 0??
    for(int i=0;i<100;i++){
        xVal[i]=i;
        yVal[0][i]=0;
        yVal[1][i]=0;
        yVal[2][i]=0;
        yVal[3][i]=0;
        yVal[4][i]=0;
        yVal[5][i]=0;
    }

    Channels[0]->setRawSamples(xVal,yVal[0],1000);
    Channels[1]->setRawSamples(xVal,yVal[1],1000);
    Channels[2]->setRawSamples(xVal,yVal[2],1000);
    Channels[3]->setRawSamples(xVal,yVal[3],1000);
    Channels[4]->setRawSamples(xVal,yVal[4],1000);
    Channels[5]->setRawSamples(xVal,yVal[5],1000);

    Channels[0]->setPen(QPen(Qt::red)); // Color de la curva
    Channels[1]->setPen(QPen(Qt::cyan));
    Channels[2]->setPen(QPen(Qt::yellow));
    Channels[3]->setPen(QPen(Qt::green));
    Channels[4]->setPen(QPen(Qt::blue));
    Channels[5]->setPen(QPen(Qt::black));

    m_Grid = new QwtPlotGrid();        // Rejilla de puntos
    m_Grid->attach(ui->Grafica);       // que se asocia al objeto QwtPl
    ui->Grafica->setAutoReplot(false); //Desactiva el autoreplot (mejora la eficiencia)

    //termina inicializacion de la grafica
   //GRAFICAS BMI:
    //GRAFICA ACELERACION
    ui->Grafica_Aceleracion->setTitle("Grafica ACELEROMETRO"); // Titulo de la grafica
    ui->Grafica_Aceleracion->setAxisTitle(QwtPlot::xBottom, "Tiempo"); // Etiqueta eje X de coordenadas
    ui->Grafica_Aceleracion->setAxisTitle(QwtPlot::yLeft, "G");    // Etiqueta eje Y de coordenadas
    //ui->Grafica->axisAutoScale(true); // Con Autoescala

    //Ojo, el rango cambia según como lo configuremos desde la función del CCS
    ui->Grafica_Aceleracion->setAxisScale(QwtPlot::yLeft, -2, 2); // Pongo esta escala ya que es la que tengo configurada en la inicializacion del BMI
    ui->Grafica_Aceleracion->setAxisScale(QwtPlot::xBottom,0,100.0);

    // Formateo de la curva, establezco numero de curvas y las asocio a la grafica
    for(int i=0;i<=2;i++){
        ChannelsAcc[i] = new QwtPlotCurve();
        ChannelsAcc[i]->attach(ui->Grafica_Aceleracion);
    }

    //Inicializacion de los valores básicos, en este caso a 0??
    for(int i=0;i<100;i++){
        xValAceleracion[i]=i;
        yValAceleracion[0][i]=0;
        yValAceleracion[1][i]=0;
        yValAceleracion[2][i]=0;
    }

    ChannelsAcc[0]->setRawSamples(xValAceleracion,yValAceleracion[0],100);
    ChannelsAcc[1]->setRawSamples(xValAceleracion,yValAceleracion[1],100);
    ChannelsAcc[2]->setRawSamples(xValAceleracion,yValAceleracion[2],100);

    ChannelsAcc[0]->setPen(QPen(Qt::red)); // Color de la curva
    ChannelsAcc[1]->setPen(QPen(Qt::cyan));
    ChannelsAcc[2]->setPen(QPen(Qt::yellow));


    m_Grid_Acc = new QwtPlotGrid();        // Rejilla de puntos
    m_Grid_Acc->attach(ui->Grafica_Aceleracion);       // que se asocia al objeto QwtPl
    ui->Grafica_Aceleracion->setAutoReplot(false); //Desactiva el autoreplot (mejora la eficiencia)


    //FIN GRAFICA ACELERACION
    //GRAFICA GIRO
    ui->grafica_Vangular->setTitle("Grafica Velocidad Angular"); // Titulo de la grafica
    ui->grafica_Vangular->setAxisTitle(QwtPlot::xBottom, "Tiempo"); // Etiqueta eje X de coordenadas
    ui->grafica_Vangular->setAxisTitle(QwtPlot::yLeft, "º/s");    // Etiqueta eje Y de coordenadas
    //ui->Grafica->axisAutoScale(true); // Con Autoescala

    //Ojo, el rango cambia según como lo configuremos desde la función del CCS
    ui->grafica_Vangular->setAxisScale(QwtPlot::yLeft, -125, +125); // Pongo esta escala ya que es la que tengo configurada en la inicializacion del BMI
    ui->grafica_Vangular->setAxisScale(QwtPlot::xBottom,0,100.0);

    // Formateo de la curva, establezco numero de curvas y las asocio a la grafica
    for(int i=0;i<=2;i++){
        ChannelsGiro[i] = new QwtPlotCurve();
        ChannelsGiro[i]->attach(ui->grafica_Vangular);
    }

    //Inicializacion de los valores básicos, en este caso a 0??
    for(int i=0;i<100;i++){
        xValGiro[i]=i;
        yValGiro[0][i]=0;
        yValGiro[1][i]=0;
        yValGiro[2][i]=0;
    }

    ChannelsGiro[0]->setRawSamples(xValGiro,yValGiro[0],100);
    ChannelsGiro[1]->setRawSamples(xValGiro,yValGiro[1],100);
    ChannelsGiro[2]->setRawSamples(xValGiro,yValGiro[2],100);

    ChannelsGiro[0]->setPen(QPen(Qt::red)); // Color de la curva
    ChannelsGiro[1]->setPen(QPen(Qt::cyan));
    ChannelsGiro[2]->setPen(QPen(Qt::yellow));


    m_Grid_Giro = new QwtPlotGrid();        // Rejilla de puntos
    m_Grid_Giro->attach(ui->grafica_Vangular);       // que se asocia al objeto QwtPl
    ui->grafica_Vangular->setAutoReplot(false); //Desactiva el autoreplot (mejora la eficiencia)

    //fin GRAFICA giro

    //Conexion de signals de los widgets del interfaz con slots propios de este objeto
    connect(ui->rojo,SIGNAL(toggled(bool)),this,SLOT(cambiaLEDs()));
    connect(ui->verde,SIGNAL(toggled(bool)),this,SLOT(cambiaLEDs()));
    connect(ui->azul,SIGNAL(toggled(bool)),this,SLOT(cambiaLEDs()));

    //connect checkboxes gpio apartado 3
    connect(ui->checkBox_GPIO4,SIGNAL(toggled(bool)),this,SLOT(cambiaGPIO()));
    connect(ui->checkBox_GPIO5,SIGNAL(toggled(bool)),this,SLOT(cambiaGPIO()));
    connect(ui->checkBox_GPIO6,SIGNAL(toggled(bool)),this,SLOT(cambiaGPIO()));
    connect(ui->checkBox_GPIO7,SIGNAL(toggled(bool)),this,SLOT(cambiaGPIO()));
    connect(ui->checkBox_Acelerometro,SIGNAL(toggled(bool)),this,SLOT(lecturaBMI()));
    connect(ui->knob_frecuencia_muestreo,SIGNAL(sliderMoved(double)),this,SLOT(lecturaBMI()));//*

    //(las demás señales y slots las he conectado por nombre)

    //Conectamos Slots del objeto "Tiva" con Slots de nuestra aplicacion (o con widgets)
    connect(&tiva,SIGNAL(statusChanged(int,QString)),this,SLOT(tivaStatusChanged(int,QString)));
    connect(&tiva,SIGNAL(messageReceived(uint8_t,QByteArray)),this,SLOT(messageReceived(uint8_t,QByteArray)));
}

MainUserGUI::~MainUserGUI() // Destructor de la clase
{
    delete ui;   // Borra el interfaz gráfico asociado a la clase
}

void MainUserGUI::on_serialPortComboBox_currentIndexChanged(const QString &arg1)
{
    activateRunButton();
}

// Funcion auxiliar de procesamiento de errores de comunicación
void MainUserGUI::processError(const QString &s)
{
    activateRunButton(); // Activa el botón RUN
    // Muestra en la etiqueta de estado la razón del error (notese la forma de pasar argumentos a la cadena de texto)
    ui->statusLabel->setText(tr("Status: Not running, %1.").arg(s));
}

// Funcion de habilitacion del boton de inicio/conexion
void MainUserGUI::activateRunButton()
{
    ui->runButton->setEnabled(true);
}


// SLOT asociada a pulsación del botón RUN
void MainUserGUI::on_runButton_clicked()
{
    //Intenta arrancar la comunicacion con la TIVA
    tiva.startRemoteLink( ui->serialPortComboBox->currentText());
}



//Slots Asociado al boton que limpia los mensajes del interfaz
void MainUserGUI::on_pushButton_clicked()
{
    ui->statusLabel->setText(tr(""));
}

void MainUserGUI::on_colorWheel_colorChanged(const QColor &arg1)
{
    //Poner aqui el codigo para pedirle al objeto "tiva" que envie el mensaje correspondiente
    //Poner aqui el codigo para pedirle al objeto "tiva" que envie el mensaje correspondiente
    MESSAGE_COLOR_PWM_PARAMETER parameter;

    //parameter.arrayRGB[0]=ui->colorWheel->arg1[0];
    parameter.arrayRGB[0]=arg1.red();
    parameter.arrayRGB[1]=arg1.green();
    parameter.arrayRGB[2]=arg1.blue();

    tiva.sendMessage(MESSAGE_COLOR_PWM,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));
}

void MainUserGUI::on_Knob_valueChanged(double value)
{ 
    //Tenemos que enviar la misma estructura que recibimos en el microcontrolador (code composer)
    MESSAGE_LED_PWM_BRIGHTNESS_PARAMETER parametro;
    parametro.rIntensity=value;
    //
    //Este mensaje se envia "mal" intencionadamente (no se corrsponde con lo que el microcontrolador pretende recibir)
    tiva.sendMessage(MESSAGE_LED_PWM_BRIGHTNESS,QByteArray::fromRawData((char *)&parametro,sizeof(parametro)));
}

void MainUserGUI::on_ADCButton_clicked()
{
    tiva.sendMessage(MESSAGE_ADC_SAMPLE,NULL,0);
}

void MainUserGUI::on_pingButton_clicked()
{
    tiva.sendMessage(MESSAGE_PING,NULL,0);
}

void MainUserGUI::cambiaLEDs(void)
{

    MESSAGE_LED_GPIO_PARAMETER parameter;

    parameter.leds.red=ui->rojo->isChecked();
    parameter.leds.green=ui->verde->isChecked();
    parameter.leds.blue=ui->azul->isChecked();

    tiva.sendMessage(MESSAGE_LED_GPIO,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));
}

//*nuevo
void MainUserGUI::on_checkBoxPWM_clicked()
{
    MESSAGE_MODE_GPIO_PWM_PARAMETER parameter;

    parameter.pwm=ui->checkBoxPWM->isChecked();//comprueba si esta con check o no

    tiva.sendMessage(MESSAGE_MODE_GPIO_PWM,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));

    ui->checkBoxPWM->setChecked(false);//restablecemos los valores del modo pwm a (no checkeado))//repasar esta parte

}


void MainUserGUI::on_checkBoxGPIO_clicked()
{
    MESSAGE_MODE_GPIO_PWM_PARAMETER parameter;

    parameter.gpio=ui->checkBoxGPIO->isChecked();

    tiva.sendMessage(MESSAGE_MODE_GPIO_PWM,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));

    ui->checkBoxGPIO->setChecked(false);//restablecemos los valores del modo gpio a (no checkeado))
}


void MainUserGUI::on_ControlAsincrono_clicked()
{
    MESSAGE_CONTROL_ASINCRONO_PARAMETER parameter;
    parameter.automatico=ui->ControlAsincrono->isChecked();//checkea si esta o no checkeado. SI esta checkeado manda false y si no true
    tiva.sendMessage(MESSAGE_CONTROL_ASINCRONO,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));
}


void MainUserGUI::on_LEER_BOTONES_2_clicked()
{
    MESSAGE_CONSULTAR_PINES_PARAMETER parameter;
    tiva.sendMessage(MESSAGE_CONSULTAR_PINES,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));
}

void MainUserGUI::on_comboBox_activated(int index)
{
    uint32_t muestras = 2 * pow(2, index);//saco el valor de promedidado HW de esta forma 2* 2 elevado al indice(Index empieza en 0)


    MESSAGE_CONFIGURAR_SOBREMUESTREO_PARAMETER parameter;
    parameter.longitud=muestras;

    tiva.sendMessage(MESSAGE_CONFIGURAR_SOBREMUESTREO,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));
}

void MainUserGUI::on_ADC_automatico_clicked()
{
    ui->ADCButton->setEnabled(!ui->ADC_automatico->isChecked());//esto sirve para deshabilitar el boton del ADC manual cuando esta el ADC automatico clickado ya que no tiene sentido pulsar los dos a la vez
    //hacer lo mismo para no poder mover la ruleta cuando está activado el modo de muestreo manual
    MESSAGE_CONFIGURAR_FRECUENCIA_MUESTREO_PARAMETER parameter;
    uint8_t indice=ui->multiplicador_frecuencia->currentIndex();
    uint8_t multiplicador=pow(10,indice);
    double rango=ui->rosca_frecuencia->value();
    //ojo de esta forma me puedo sobrepasar de 5khz??
    parameter.frecuencia=double(multiplicador)*rango;
    parameter.automatico=ui->ADC_automatico->isChecked();//checkea si esta o no checkeado. SI esta checkeado manda false y si no true
    tiva.sendMessage(MESSAGE_CONFIGURAR_FRECUENCIA_MUESTREO,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));
}

void MainUserGUI::on_rosca_frecuencia_valueChanged(double value)
{
    MESSAGE_CONFIGURAR_FRECUENCIA_MUESTREO_PARAMETER parameter;
    uint8_t indice=ui->multiplicador_frecuencia->currentIndex();
    uint8_t multiplicador=pow(10,indice);
    parameter.frecuencia=double(multiplicador)*value;
    parameter.automatico=ui->ADC_automatico->isChecked();//checkea si esta o no checkeado. SI esta checkeado manda false y si no true
    tiva.sendMessage(MESSAGE_CONFIGURAR_FRECUENCIA_MUESTREO,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));
}

void MainUserGUI::on_multiplicador_frecuencia_activated(int index)
{
    MESSAGE_CONFIGURAR_FRECUENCIA_MUESTREO_PARAMETER parameter;
    double rango=ui->rosca_frecuencia->value();
    parameter.frecuencia=double(pow(10,index))*rango;//10 elevado a index, que empieza en 0,ya que en este caso necesitamos frecuencias del orden de los khz
    parameter.automatico=ui->ADC_automatico->isChecked();//checkea si esta o no checkeado. SI esta checkeado manda false y si no true
    tiva.sendMessage(MESSAGE_CONFIGURAR_FRECUENCIA_MUESTREO,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));
}

void MainUserGUI::cambiaGPIO()
{
    MESSAGE_GPIO4_7_PARAMETER parameter;

    parameter.gpio4a7.value0a3=0;
    parameter.gpio4a7.gpio4=ui->checkBox_GPIO4->isChecked();
    parameter.gpio4a7.gpio5=ui->checkBox_GPIO5->isChecked();
    parameter.gpio4a7.gpio6=ui->checkBox_GPIO6->isChecked();
    parameter.gpio4a7.gpio7=ui->checkBox_GPIO7->isChecked();


    tiva.sendMessage(MESSAGE_GPIO4_7,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));
}

void MainUserGUI::lecturaBMI()
{
       //Crear nuevo mensaje para configurar la frecuencia de muestreo del BMI e iniciar el muestreo,
    //que por defecto se iniciará con frecuencia de muestro 5Hz que es la minimo que admite el knob configurado
       MESSAGE_MUESTRE0_y_FRECUENCIA_PARAMETER parameter;
       parameter.activar_muestreo=ui->checkBox_Acelerometro->isChecked();
       parameter.frecuencia=ui->knob_frecuencia_muestreo->value();
        tiva.sendMessage(MESSAGE_MUESTRE0_y_FRECUENCIA,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));
}

void MainUserGUI::on_comboBox_Acc_activated(int index)
{
    //2,4,8,16
    uint8_t muestras = pow(2, index+1);//saco el valor del indice de esta forma 2* 2 elevado al indice(index empieza en 0)


    MESSAGE_ACC_CAMBIAR_RANGO_PARAMETER parameter;
    parameter.rango_acc=muestras;
  //cambio la escala de la gráfica:
    ui->Grafica_Aceleracion->setAxisScale(QwtPlot::yLeft, -muestras, muestras);
   // ui->Grafica_Aceleracion->replot();
    tiva.sendMessage(MESSAGE_ACC_CAMBIAR_RANGO,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));
}

void MainUserGUI::on_comboBox_3_activated(int index)//para configurar rango giroscopio
{
    //125,250,500,1000,2000
    MESSAGE_GIRO_CAMBIAR_RANGO_PARAMETER parameter;
     parameter.rango_giro = 125 * pow(2,index);

//cambio la escala de la gráfica
    ui->grafica_Vangular->setAxisScale(QwtPlot::yLeft, -parameter.rango_giro, parameter.rango_giro);
    tiva.sendMessage(MESSAGE_GIRO_CAMBIAR_RANGO,QByteArray::fromRawData((char *)&parameter,sizeof(parameter)));
}

//
//**** Slot asociado a la recepción de mensajes desde la TIVA ********/
//Está conectado a una señale generada por el objeto TIVA de clase QTivaRPC (se conecta en el constructor de la ventana, más arriba en este fichero))
//Se pueden añadir los que casos que quieran para completar la aplicación

void MainUserGUI::messageReceived(uint8_t message_type, QByteArray datos)
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
            ui->statusLabel->setText(tr("Status: Comando rechazado,%1").arg(parametro.command));
            }
            else
            {
                ui->statusLabel->setText(tr("Status: MSG %1, recibí %2 B y esperaba %3").arg(message_type).arg(datos.size()).arg(sizeof(parametro)));
            }
        }
        break;

        case MESSAGE_ADC_SAMPLE:
        {    // Este caso trata la recepcion de datos del ADC desde la TIVA
            MESSAGE_ADC_SAMPLE_PARAMETER parametro;
            if (check_and_extract_command_param(datos.data(), datos.size(), &parametro, sizeof(parametro))>0)
            {

                ui->lcdCh1->display(((double)parametro.channel[0])*3.3/4096.0);
                ui->lcdCh2->display(((double)parametro.channel[1])*3.3/4096.0);
                ui->lcdCh3->display(((double)parametro.channel[2])*3.3/4096.0);
                ui->lcdCh4->display(((double)parametro.channel[3])*3.3/4096.0);
                ui->lcdCh5->display(((double)parametro.channel[4])*3.3/4096.0);
                ui->lcdCh6->display(((double)parametro.channel[5])*3.3/4096.0);
                ui->mitermometro->display(147.5 - (((double)parametro.channel[6])*75*3.3/4096.0));//
                //147.5 - ((75 * (VREFP - VREFN) × ADCCODE) / 4096)
               //aqui cada 10 muestras también represento en la gráfica???
            }
            else
            {   //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                ui->statusLabel->setText(tr("Status: MSG %1, recibí %2 B y esperaba %3").arg(message_type).arg(datos.size()).arg(sizeof(parametro)));
            }

        }
        break;
        case MESSAGE_CONSULTAR_PINES://*
        {
            MESSAGE_CONSULTAR_PINES_PARAMETER parametro;
            if (check_and_extract_command_param(datos.data(), datos.size(), &parametro, sizeof(parametro))>0)
            {
               /* if(parametro.PF0==1)//BOTÓN DERECHO
                {
                    ui->boton_der->setEnabled(true);
                }
                else
                {
                    ui->boton_der->setEnabled(false);
                }
                if(parametro.PF4==1)
                {
                    ui->boton_izq->setEnabled(true);
                }
                else
                {
                    ui->boton_izq->setEnabled(false);
                }*/
                //asi más corto y no hay que comprobar con condiciones
                ui->boton_izq->setEnabled(parametro.PF4);
                ui->boton_der->setEnabled(parametro.PF0);
            }
            else
            {   //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                ui->statusLabel->setText(tr("SStatus: MSG %1, recibí %2 B y esperaba %3").arg(message_type).arg(datos.size()).arg(sizeof(parametro)));
            }

        }
        break;
        case   MESSAGE_CONTROL_ASINCRONO:
        {
            MESSAGE_CONTROL_ASINCRONO_PARAMETER parametro;
            if (check_and_extract_command_param(datos.data(), datos.size(), &parametro, sizeof(parametro))>0)
            {

                ui->boton_izq->setEnabled(parametro.PF4);
                ui->boton_der->setEnabled(parametro.PF0);
            }
            else
            {   //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                ui->statusLabel->setText(tr("Status: MSG %1, recibí %2 B y esperaba %3").arg(message_type).arg(datos.size()).arg(sizeof(parametro)));
            }
        }
        break;
        case MESSAGE_ADC_SAMPLE_AUTO:
        {

            MESSAGE_ADC_SAMPLE_AUTO_PARAMETER parametro2;

            if (check_and_extract_command_param(datos.data(), datos.size(), &parametro2, sizeof(parametro2))>0)
            {
            ui->lcdCh1->display(((double)parametro2.channel[0][9])*3.3/4096.0);//Aqui represento la ultima muestra de cada canal(de las 10 muestras que se envian)
            ui->lcdCh2->display(((double)parametro2.channel[1][9])*3.3/4096.0);
            ui->lcdCh3->display(((double)parametro2.channel[2][9])*3.3/4096.0);
            ui->lcdCh4->display(((double)parametro2.channel[3][9])*3.3/4096.0);
            ui->lcdCh5->display(((double)parametro2.channel[4][9])*3.3/4096.0);
            ui->lcdCh6->display(((double)parametro2.channel[5][9])*3.3/4096.0);
            //ui->mitermometro->display(147.5 - (((double)parametro.term)*75*3.3/4096.0));//La especificacion no me pide en el caso automatico el termometro
            //147.5 - ((75 * (VREFP - VREFN) × ADCCODE) / 4096)

            //para representar en la grafica

                //yval esta definido en mainusergui.h
                    //Como represento el tiempo en el eje X??
                 //recalcula los valores de la gráfica

            //con esto voy desplazando los valores 10 posiciones hacia la derecha cada vez que entra en esta función
                    for(int j=1000; j>=10; j--)
                       {
                    for(int i=0; i<6 ;i++)
                          {
                            yVal[i][j]=yVal[i][j-10];//??
                          }
                       }
                //asigno los valores a representar en la gráfica(en la gráfica no se representa el termometro)
                       for (int j=0; j<10;j++)
                       {
                           for(int i=0;i<6;i++)
                           {
                               yVal[i][j]=(((double)parametro2.channel[i][j])*2/4096.0);
                           }
                       }
                        ui->Grafica->replot(); //Refresca la grafica una vez actualizados los valores

            }
           else
            {
                //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                ui->statusLabel->setText(tr("Status: MSG %1, recibí %2 B y esperaba %3").arg(message_type).arg(datos.size()).arg(sizeof(parametro2)));
            }
        }
        break;
        case MESSAGE_ADC_SAMPLE_DIFF:
        {
            MESSAGE_ADC_SAMPLE_DIFF_PARAMETER parametro;
            if (check_and_extract_command_param(datos.data(), datos.size(), &parametro, sizeof(parametro))>0)
            {
                ui->Diferencia1->display(((double)parametro.channeldiff[0][19]-2048)*2*3.3/4096.0);//Aqui represento la ultima muestra de cada canal(de las 20 muestras que se envian)
                ui->Diferencia2->display(((double)parametro.channeldiff[1][19]-2048)*2*3.3/4096.0);
                ui->Diferencia3->display(((double)parametro.channeldiff[2][19]-2048)*2*3.3/4096.0);
               //voy desplazando todas las muestras que ya tengo 20 muestras a la derecha para que entren las nuevas
                for(int j=1000; j>=20; j--)
                {
                  for(int i=0; i<3 ;i++)
                  {
                    yValDiff[i][j]=yValDiff[i][j-20];//??
                  }
                }
               //asigno los valores a representar en la gráfica(en la gráfica no se representa el termometro)
                for (int j=0; j<20;j++)
                {
                  for(int i=0;i<3;i++)
                  {                               //como el 2048 es 0 se lo restamos(pag 809 datasheet). Se multiplica por dos ya que el margen va de -3.3V a 3.3V
                    yValDiff[i][j]=(((double)parametro.channeldiff[i][j]-2048)*2*3.3/4096.0);
                  }
                }
                ui->GraficaDiff->replot(); //Refresca la grafica una vez actualizados los valores
            }
            else
            {
                //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                ui->statusLabel->setText(tr("Status: MSG %1, recibí %2 B y esperaba %3").arg(message_type).arg(datos.size()).arg(sizeof(parametro)));
            }
        }break;
        case MESSAGE_GPIO0_3:
        {
            MESSAGE_GPIO0_3_PARAMETER parametro;
            if (check_and_extract_command_param(datos.data(), datos.size(), &parametro, sizeof(parametro))>0)
            {
                //gpio0a3 es un entero de 8 bits (me interesan en este caso los 4 de menor peso)

                //>>:de esta forma desplazo los bites a la derecha para tener el que me interese en cada caso en la posicion menos significativa
                //&1 se usa para obtener el valor del bit menos significativo

                ui->led_GPIO0->setEnabled((parametro.gpio0a3)&1);
                ui->led_GPIO1->setEnabled(((parametro.gpio0a3)>>1)&1);
                ui->led_GPIO2->setEnabled(((parametro.gpio0a3)>>2)&1);
                ui->led_GPIO3->setEnabled(((parametro.gpio0a3)>>3)&1);
            }
            else
            {
                //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                ui->statusLabel->setText(tr("Status: MSG %1, recibí %2 B y esperaba %3").arg(message_type).arg(datos.size()).arg(sizeof(parametro)));
            }

        }break;

        case MESSAGE_ACELERACION_y_GIRO:
        {
            MESSAGE_ACELERACION_y_GIRO_PARAMETER parametro;
            if(check_and_extract_command_param(datos.data(), datos.size(), &parametro, sizeof(parametro))>0)
            {//Aqui representamos las dos graficas a la vez ya que recibimos los valores simultaneamente
                //voy desplazando todas las muestras a la derecha para que entren las nuevas, en este caso no acumulo muestras antes de enviar por lo q
                //que las recibo de 1 en 1 (de cada eje)

                uint8_t rango_giro=125*pow(2,ui->comboBox_3->currentIndex());
                uint8_t rango_acc=(pow(2,ui->comboBox_Acc->currentIndex()+1));

                for(int j=100; j>=1; j--)
                {
                    for(int i=0; i<3 ;i++)
                    {
                        yValAceleracion[i][j]=yValAceleracion[i][j-1];//
                        yValGiro[i][j]=yValGiro[i][j-1];
                    }
                }


                    //Como el acelerometro devuleve valores de 16 bits-> rango 65536 (va de -32768 a 32768). Al hacer la transformacion
                    //hay que tener en cuenta el rango que esta puesto ya que si el rango es +-2g 2g es 32764, pero si es +-4g , 4g tambén es 32764!!!
                    //EL giroscopio:
                    for(int i=0;i<3;i++)
                    {
                        yValAceleracion[i][0]=(((double)parametro.aceleracion[i])*rango_acc/32768.0);
                        yValGiro[i][0]=(((double)parametro.giro[i])*rango_giro/32768.0);

                    }

                ui->Grafica_Aceleracion->replot(); //Refresca la grafica una vez actualizados los valores
                   ui->Grafica_Aceleracion->replot();
            }
            else
            {
                //Si el tamanho de los datos no es correcto emito la senhal statusChanged(...) para reportar un error
                ui->statusLabel->setText(tr("Status: MSG %1, recibí %2 B y esperaba %3").arg(message_type).arg(datos.size()).arg(sizeof(parametro)));
            }
        } break;
        default:
            //Notifico que ha llegado un tipo de mensaje desconocido
            ui->statusLabel->setText(tr("Status: Recibido mensaje desconocido %1").arg(message_type));
        break; //Innecesario
    }
}

//Este Slot lo hemos conectado con la señal statusChange de la TIVA, que se activa cuando
//El puerto se conecta o se desconecta y cuando se producen determinados errores.
//Esta función lo que hace es procesar dichos eventos
void MainUserGUI::tivaStatusChanged(int status,QString message)
{
    switch (status)
    {
        case TivaRemoteLink::TivaConnected:

            //Caso conectado..
            // Deshabilito el boton de conectar
            ui->runButton->setEnabled(false);

            // Se indica que se ha realizado la conexión en la etiqueta 'statusLabel'
            ui->statusLabel->setText(tr("Ejecucion, conectado al puerto %1.").arg(ui->serialPortComboBox->currentText()));

            //   Y se habilitan los controles deshabilitados
            ui->pingButton->setEnabled(true);

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
        break;
        default:
            //Otros errores (por ejemplo, abriendo el puerto)
            processError(tiva.getLastErrorMessage());
    }
}





















