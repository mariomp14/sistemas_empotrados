#ifndef MAINUSERGUI_H
#define MAINUSERGUI_H

#include <QWidget>
#include <QtSerialPort/qserialport.h>
#include <QMessageBox>
#include "tiva_remotelink.h"
//librerias para las curvas
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <QPen>

namespace Ui {
class MainUserGUI;
}

class MainUserGUI : public QWidget
{
    Q_OBJECT
    
public: 
    explicit MainUserGUI(QWidget *parent = 0);
    ~MainUserGUI();
    
private slots:
    // slots privados asociados mediante "connect" en el constructor
    void cambiaLEDs();
    void tivaStatusChanged(int status,QString message);
    void messageReceived(uint8_t type, QByteArray datos);
    void cambiaGPIO();
    void lecturaBMI();

    //Slots asociados por nombre
    void on_runButton_clicked();    
    void on_serialPortComboBox_currentIndexChanged(const QString &arg1);
    void on_pushButton_clicked();    
    void on_colorWheel_colorChanged(const QColor &arg1);
    void on_Knob_valueChanged(double value);
    void on_ADCButton_clicked();
    void on_pingButton_clicked();


    void on_checkBoxPWM_clicked();

    void on_checkBoxGPIO_clicked();

    void on_ControlAsincrono_clicked();

    void on_LEER_BOTONES_2_clicked();

    void on_comboBox_activated(int index);

    void on_rosca_frecuencia_valueChanged(double value);

    void on_multiplicador_frecuencia_activated(int index);

    void on_ADC_automatico_clicked();

    void on_comboBox_Acc_activated(int index);

    void on_comboBox_3_activated(int index);

private:
    // funciones privadas
    void processError(const QString &s);
    void activateRunButton();

private:
    //Componentes privados
    Ui::MainUserGUI *ui;
    int transactionCount;
    QMessageBox ventanaPopUp;
    TivaRemoteLink tiva; //Objeto para gestionar la comunicacion de mensajes con el microcontrolador

    // Para las graficas
    double xVal[1000]; //valores eje X, al ser el tiempo vamos a tener un unico valor del eje x para todos los canales
    double yVal[6][1000]; //valores ejes Y, 6 valores diferentes del ejeX, uno para cada canal que se muestrea (va de 0 a 5 el índice). Hasta 1000 muestras de cada canal
    QwtPlotCurve *Channels[6]; //Curvas
    QwtPlotGrid  *m_Grid; //Cuadricula

    //Grafica Diferencial
    double xValDiff[1000]; //valores eje X, al ser el tiempo vamos a tener un unico valor del eje x para todos los canales
    double yValDiff[3][1000]; //valores ejes Y, 6 valores diferentes del ejeX, uno para cada canal que se muestrea (va de 0 a 5 el índice). Hasta 1000 muestras de cada canal
    QwtPlotCurve *ChannelsDiff[3]; //Curvas
    QwtPlotGrid  *m_Grid_Diff; //Cuadricula

    //Grafica Aceleracion
    double xValAceleracion[100]; //valores eje X, al ser el tiempo vamos a tener un unico valor del eje x para todos los canales
    double yValAceleracion[3][100]; //valores ejes Y, 6 valores diferentes del ejeX, uno para cada canal que se muestrea (va de 0 a 5 el índice). Hasta 1000 muestras de cada canal
    QwtPlotCurve *ChannelsAcc[3]; //Curvas
    QwtPlotGrid  *m_Grid_Acc; //Cuadricula

    //Grafica Giro
    double xValGiro[100]; //valores eje X, al ser el tiempo vamos a tener un unico valor del eje x para todos los canales
    double yValGiro[3][100]; //valores ejes Y, 6 valores diferentes del ejeX, uno para cada canal que se muestrea (va de 0 a 5 el índice). Hasta 1000 muestras de cada canal
    QwtPlotCurve *ChannelsGiro[3]; //Curvas
    QwtPlotGrid  *m_Grid_Giro; //Cuadricula

};

#endif // GUIPANEL_H
