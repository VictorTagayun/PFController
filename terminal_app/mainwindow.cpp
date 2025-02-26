/**
 * @file mainwindow.c
 * @author Stanislav Karpikov
 * @brief Main application window management
 */

/** @addtogroup app_main
 * @{
 */

/*--------------------------------------------------------------
                       INCLUDES
--------------------------------------------------------------*/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"
#include <QMessageBox>
#include <QMessageBox>
#include <QDateTime>
#include <QGraphicsDropShadowEffect>
#include <QListWidgetItem>
#include "htmldelegate.h"
#include "device_definition.h"
#include "interface_definitions.h"
#include "interface_messaging.h"

#include <sstream> /* std::stringstream */
#include <iomanip> /* std::setfill, std::setw */

/*--------------------------------------------------------------
                       NAMESPACES
--------------------------------------------------------------*/

using namespace PFCconfig;
using namespace PFCconfig::ADC;
using namespace PFCconfig::Interface;
using namespace PFCconfig::Events;
using namespace InterfaceDefinitions;
using namespace InterfaceMessaging;

/*--------------------------------------------------------------
                       PRIVATE DATA
--------------------------------------------------------------*/


/*--------------------------------------------------------------
                       CLASS FUNCTIONS
--------------------------------------------------------------*/

MainWindow::TableCalibrationRows& operator++(MainWindow::TableCalibrationRows& row, int)
{
    row = static_cast<MainWindow::TableCalibrationRows>( enum_uint(row) + 1 );
    return row;
}

/*--------------------------------------------------------------
                       CLASS FUNCTIONS
--------------------------------------------------------------*/

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      _ui(new Ui::MainWindow),
      _pfc(new PFC),
      _pfc_settings(new PFCsettings),
      _page_filters(_ui, _pfc_settings, _pfc),
      _page_oscillog(_ui, _pfc_settings, _pfc),
      _page_main(_ui, _pfc_settings, _pfc),
      _last_index_events(0),
      _port_settings(new SettingsDialog),
      _connected(false),
      _btns_edit()
{
    /* Init the UI */
    _ui->setupUi(this);

    _ui->actionConnect->setEnabled(true);
    _ui->actionDisconnect->setEnabled(false);
    _ui->actionQuit->setEnabled(true);
    _ui->actionConfigure->setEnabled(true);

    initInterfaceConnections();

    qRegisterMetaType<QSerialPort::DataBits>("QSerialPort::DataBits");
    qRegisterMetaType<QSerialPort::Parity>("QSerialPort::Parity");
    qRegisterMetaType<QSerialPort::StopBits>("QSerialPort::StopBits");
    qRegisterMetaType<QSerialPort::FlowControl>("QSerialPort::FlowControl");
    qRegisterMetaType<std::vector<uint8_t>>("std::vector<uint8_t>");
    qRegisterMetaType<InterfacePackage*>("PackageCommand*");
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<std::string>("std::string");

    _ui->groupState->installEventFilter(this);
    _ui->groupNetworkParameters->installEventFilter(this);
    _ui->OscillogPlot->installEventFilter(this);
    _ui->pageCalibrations->installEventFilter(this);
    _ui->pageCapacitors->installEventFilter(this);
    _ui->pageProtection->installEventFilter(this);
    _ui->pageFilters->installEventFilter(this);

    /* Init settings window and show it */
    QMetaObject::invokeMethod(_port_settings, "show", Qt::QueuedConnection);

    /* Init PFC device subsystem */
    connect(_pfc, &PFC::interfaceConnected, this, &MainWindow::deviceConnected);
    connect(_pfc, &PFC::interfaceDisconnected, this, &MainWindow::deviceDisconnected);

    connect(_pfc, &PFC::setConnection, this, &MainWindow::setConnection);

    connect(_pfc, &PFC::setNetVoltage, this, &MainWindow::setNetVoltage);
    connect(_pfc, &PFC::setSwitchOnOff, this, &MainWindow::setSwitchOnOff);
    connect(_pfc, &PFC::setNetVoltageRAW, this, &MainWindow::setNetVoltageRAW);
    connect(_pfc, &PFC::setNetParams, this, &MainWindow::setNetParams);

    connect(_pfc, &PFC::setEvents, this, &MainWindow::setEvents);

    connect(_pfc, &PFC::message, this, &MainWindow::message);

    connect(_pfc, &PFC::setSettingsCalibrations,
            this, &MainWindow::setSettingsCalibrations);
    connect(_pfc, &PFC::setSettingsProtection,
            this, &MainWindow::setSettingsProtection);
    connect(_pfc, &PFC::setSettingsCapacitors,
            this, &MainWindow::setSettingsCapacitors);

    connect(_pfc, &PFC::ansSettingsCalibrations,
            this, &MainWindow::ansSettingsCalibrations);
    connect(_pfc, &PFC::ansSettingsProtection,
            this, &MainWindow::ansSettingsProtection);
    connect(_pfc, &PFC::ansSettingsCapacitors,
            this, &MainWindow::ansSettingsCapacitors);

    connect(this, &MainWindow::updateNetVoltage,
            _pfc, &PFC::updateNetVoltage);
    connect(this, &MainWindow::updateNetVoltageRAW,
            _pfc, &PFC::updateNetVoltageRAW);
    connect(this, &MainWindow::updateNetParams,
            _pfc, &PFC::updateNetParams);

    connect(this, &MainWindow::updateEvents,
            _pfc, &PFC::updateEvents);

    connect(this, &MainWindow::updateSettingsCalibrations,
            _pfc, &PFC::updateSettingsCalibrations);
    connect(this, &MainWindow::updateSettingsProtection,
            _pfc, &PFC::updateSettingsProtection);
    connect(this, &MainWindow::updateSettingsCapacitors,
            _pfc, &PFC::updateSettingsCapacitors);

    connect(this, &MainWindow::writeSettingsCalibrations,
            _pfc, &PFC::writeSettingsCalibrations);
    connect(this, &MainWindow::writeSettingsProtection,
            _pfc, &PFC::writeSettingsProtection);
    connect(this, &MainWindow::writeSettingsCapacitors,
            _pfc, &PFC::writeSettingsCapacitors);

    connect(this, &MainWindow::writeSwitchOnOff,
            _pfc, &PFC::writeSwitchOnOff);

    connect(&_timer_voltage, &QTimer::timeout,
            this, &MainWindow::timerupdateNetVoltage);
    connect(&_timer_raw, &QTimer::timeout,
            this, &MainWindow::timerupdateNetVoltageRaw);
    connect(&_timer_oscillog, &QTimer::timeout,
            this, &MainWindow::timerOscillog);
    connect(&_timer_settings_calibrations, &QTimer::timeout,
            this, &MainWindow::timerSettingsCalibrations);
    connect(&_timer_settings_capacitors, &QTimer::timeout,
            this, &MainWindow::timerSettingsCapacitors);
    connect(&_timer_settings_protection, &QTimer::timeout,
            this, &MainWindow::timerSettingsProtection);
    connect(&_timer_settings_filters, &QTimer::timeout,
            this, &MainWindow::timerSettingsFilters);
    connect(&_timer_main_params, &QTimer::timeout,
            this, &MainWindow::timerNetParams);
    connect(&_timer_state, &QTimer::timeout,
            this, &MainWindow::timerWorkState);
    connect(&_timer_version, &QTimer::timeout,
            this, &MainWindow::timerVersion);
    connect(&_timer_events, &QTimer::timeout,
            this, &MainWindow::timerEvents);

    /* Init window pages */
    _page_main.pageMainInit();
    _page_oscillog.pageOscillogInit();
    pageSettingsCalibrationsInit();
    pageSettingsCapacitorsInit();
    pageSettingsProtectionInit();
    _page_filters.pageSettingsFiltersInit();

    _timer_events.start(EVENTS_TIMER_TIMEOUT);

    _ui->listLog->setItemDelegate(new HtmlDelegate);
}

void MainWindow::setFilter(QEvent* event, QObject* object, QWidget* ui_obj, QTimer* obj, std::chrono::milliseconds timeout)
{
    if (object == ui_obj)
    {
        if (event->type() == QEvent::Show)
        {
            std::chrono::milliseconds timeout_apply = timeout +
                                                ((static_cast<std::chrono::milliseconds>(qrand()) % (timeout / 4)) - (timeout / 4));

            obj->start(timeout_apply);
        }
        if (event->type() == QEvent::Hide)
        {
            obj->stop();
        }
    }
}

std::string MainWindow::stringWithColor(std::string str, std::string color)
{
    std::string ret_str;
    ret_str = "<font color=" + color + ">" + str + "</font>";
    return ret_str;
}

void MainWindow::setConnection(bool connected)
{
    _connected = connected;
    if (!_connected)
    {
        _last_index_events = 0;
    }
}

void MainWindow::filterApply(float &A, float B)
{
    A = A * FCOEFF + B * (1 - FCOEFF);
}

void MainWindow::timerupdateNetVoltage(void)
{
    if (_connected) emit updateNetVoltage();
}
void MainWindow::timerupdateNetVoltageRaw(void)
{
    if (_connected) emit updateNetVoltageRAW();
}
void MainWindow::timerSettingsCapacitors(void)
{
    if (_connected) emit updateSettingsCapacitors();
}
void MainWindow::timerSettingsProtection(void)
{
    if (_connected) emit updateSettingsProtection();
}
void MainWindow::timerSettingsFilters(void)
{
    if (_connected) emit _page_filters.update();
}
void MainWindow::timerSettingsCalibrations(void)
{
    if (_connected) emit updateSettingsCalibrations();
}

void MainWindow::timerEvents(void)
{
    if (_connected) emit updateEvents(_last_index_events);
}

void MainWindow::timerWorkState(void)
{
    /* Frequent update */
    if (_connected) _page_main.update();
}

void MainWindow::timerVersion(void)
{
    /* Low frequency update update */
    _page_main.update();
}

void MainWindow::timerNetParams(void)
{
    if (_connected) emit updateNetParams();
}

void MainWindow::timerOscillog(void)
{
    if (_connected) _page_oscillog.update();
}

bool MainWindow::eventFilter(QObject* object, QEvent* event)
{
    setFilter(event, object, _ui->groupNetworkParameters, &_timer_main_params, TIMEOUT_UPDATE_MAIN_PARAMS);
    setFilter(event, object, _ui->groupNetworkParameters, &_timer_voltage, TIMEOUT_UPDATE_VOLTAGES);
    setFilter(event, object, _ui->groupNetworkParameters, &_timer_raw, TIMEOUT_UPDATE_ADC_RAW);
    setFilter(event, object, _ui->groupNetworkParameters, &_timer_state, TIMEOUT_UPDATE_STATE);
    setFilter(event, object, _ui->groupState, &_timer_version, TIMEOUT_UPDATE_VERSION);
    setFilter(event, object, _ui->OscillogPlot, &_timer_oscillog, TIMEOUT_UPDATE_OSCILLOG);
    setFilter(event, object, _ui->pageCalibrations, &_timer_settings_calibrations, TIMEOUT_UPDATE_SETTINGS_CALIBRATIONS);
    setFilter(event, object, _ui->pageCapacitors, &_timer_settings_capacitors, TIMEOUT_UPDATE_SETTINGS_CAPACITORS);
    setFilter(event, object, _ui->pageProtection, &_timer_settings_protection, TIMEOUT_UPDATE_SETTINGS_PROTECTION);
    setFilter(event, object, _ui->pageFilters, &_timer_settings_filters, TIMEOUT_UPDATE_SETTINGS_FILTERS);
    return QMainWindow::eventFilter(object, event);
}

void MainWindow::deviceConnected(void)
{
    _ui->actionConnect->setEnabled(false);
    _ui->actionDisconnect->setEnabled(true);
    _ui->actionConfigure->setEnabled(false);
}

void MainWindow::deviceDisconnected(void)
{
    _ui->actionConnect->setEnabled(true);
    _ui->actionDisconnect->setEnabled(false);
    _ui->actionConfigure->setEnabled(true);
}

void MainWindow::setEvents(std::list<EventRecord> ev)
{
    foreach (EventRecord event, ev)
    {
        if (event.unix_time_s_ms >= _last_index_events)
        {
            _last_index_events = event.unix_time_s_ms + 1;
        }
        if (_last_index_events > TIME_MAX_VALUE)
        {
            _last_index_events = 0;
        }
        char Phases[3] = {'A', 'B', 'C'};
        std::string ADCchannels[] = {
            "ADC_UD",
            "ADC_U_A",
            "ADC_U_B",
            "ADC_U_C",
            "ADC_I_A",
            "ADC_I_B",
            "ADC_I_C",
            "ADC_I_ET",
            "ADC_I_TEMP1",
            "ADC_I_TEMP2",
            "ADC_EMS_A",
            "ADC_EMS_B",
            "ADC_EMS_C",
            "ADC_EMS_I",
        };
        std::stringstream message_stream;
        message_stream << std::fixed << std::setprecision(2);
        SubEventPower subtype = static_cast<SubEventPower>((event.type >> 16) & 0xFFFF);

        QDateTime timestamp;
        timestamp.setTime_t(static_cast<uint>(event.unix_time_s_ms / 1000));

        std::stringstream date_stream;
        date_stream << timestamp.toString("dd.MM.yyyy hh:mm:ss").toStdString();
        date_stream << ".";
        date_stream << std::setfill('0') << std::setw(4);
        date_stream << event.unix_time_s_ms % 1000;

        message_stream << stringWithColor(date_stream.str(), LIGHT_GREY);

        switch (static_cast<EventType>(event.type & 0xFFFF))
        {
            case EventType::EVENT_TYPE_POWER:
                message_stream << stringWithColor(" - Power - ", DARK_GREEN);
                switch (subtype)
                {
                    case SubEventPower::SUB_EVENT_TYPE_POWER_ON:
                        message_stream << "Power ON";
                        break;
                    default:
                        message_stream << stringWithColor(" - Unknown event! ", DARK_RED);
                        break;
                }
                break;
            case EventType::EVENT_TYPE_CHANGESTATE:
                message_stream << stringWithColor(" - State - ", DARK_GREEN);
                switch (static_cast<PFCstate>(subtype))
                {
                    case PFCstate::PFC_STATE_INIT:
                        message_stream << STRING_PFC_STATE_INIT;
                        break;
                    case PFCstate::PFC_STATE_STOP:
                        message_stream << STRING_PFC_STATE_STOP;
                        break;
                    case PFCstate::PFC_STATE_SYNC:
                        message_stream << STRING_PFC_STATE_SYNC;
                        break;
                    case PFCstate::PFC_STATE_PRECHARGE_PREPARE:
                        message_stream << STRING_PFC_STATE_PRECHARGE_PREPARE;
                        break;
                    case PFCstate::PFC_STATE_PRECHARGE:
                        message_stream << STRING_PFC_STATE_PRECHARGE;
                        break;
                    case PFCstate::PFC_STATE_MAIN:
                        message_stream << STRING_PFC_STATE_MAIN;
                        break;
                    case PFCstate::PFC_STATE_PRECHARGE_DISABLE:
                        message_stream << STRING_PFC_STATE_PRECHARGE_DISABLE;
                        break;
                    case PFCstate::PFC_STATE_WORK:
                        message_stream << STRING_PFC_STATE_WORK;
                        break;
                    case PFCstate::PFC_STATE_CHARGE:
                        message_stream << STRING_PFC_STATE_CHARGE;
                        break;
                    case PFCstate::PFC_STATE_TEST:
                        message_stream << STRING_PFC_STATE_TEST;
                        break;
                    case PFCstate::PFC_STATE_STOPPING:
                        message_stream << STRING_PFC_STATE_STOPPING;
                        break;
                    case PFCstate::PFC_STATE_FAULTBLOCK:
                        message_stream << stringWithColor(STRING_PFC_STATE_FAULTBLOCK, DARK_RED);
                        break;
                    default:
                        message_stream << STRING_PFC_STATE_UNKNOWN;
                        break;
                }
                break;
            case EventType::EVENT_TYPE_PROTECTION:
                message_stream << stringWithColor(" - Protection ", DARK_RED);
                switch (static_cast<SubEventProtection>(subtype))
                {
                    case SubEventProtection::SUB_EVENT_TYPE_PROTECTION_UCAP_MIN:
                        message_stream << " - The voltage of the capacitor is too low during operation ";
                        message_stream << "(" << event.info + 1 << ") : " << event.value << " V";
                        break;
                    case SubEventProtection::SUB_EVENT_TYPE_PROTECTION_UCAP_MAX:
                        message_stream << " - The voltage of the capacitor is too high ";
                        message_stream << "(" << event.info + 1 << ") : " << event.value << " V";
                        break;
                    case SubEventProtection::SUB_EVENT_TYPE_PROTECTION_TEMPERATURE:
                        message_stream << " - The temperature is too high: ";
                        message_stream << event.value << " °С";
                        break;
                    case SubEventProtection::SUB_EVENT_TYPE_PROTECTION_U_MIN:
                        message_stream << " - The voltage is too low at the phase ";
                        message_stream << Phases[event.info] << ": " << event.value << " V";
                        break;
                    case SubEventProtection:: SUB_EVENT_TYPE_PROTECTION_U_MAX:
                        message_stream << " - The voltage is too high at the phase ";
                        message_stream << Phases[event.info] << ": " << event.value << " V";
                        break;
                    case SubEventProtection::SUB_EVENT_TYPE_PROTECTION_F_MIN:
                        message_stream << " - The grid frequency is too low: ";
                        message_stream << event.value << "Гц";
                        break;
                    case SubEventProtection::SUB_EVENT_TYPE_PROTECTION_F_MAX:
                        message_stream << " - The grid frequency is too high: ";
                        message_stream << event.value << "Гц";
                        break;
                    case SubEventProtection::SUB_EVENT_TYPE_PROTECTION_IPFC_MAX_RMS:
                        message_stream << " - The RMS current exceeded the threshold on the phase ";
                        message_stream << Phases[event.info] << ": " << event.value << " A";
                        break;
                    case SubEventProtection::SUB_EVENT_TYPE_PROTECTION_IAFG_MAX_PEAK:
                        message_stream << " - The peak current exceeded the threshold on the phase ";
                        message_stream << Phases[event.info] << ": " << event.value << " A";
                        break;
                    case SubEventProtection::SUB_EVENT_TYPE_PROTECTION_PHASES:
                        message_stream << " - Wrong phase rotation ";
                        break;
                    case SubEventProtection::SUB_EVENT_TYPE_PROTECTION_ADC_OVERLOAD:
                        message_stream << " - ADC overload on channel ";
                        message_stream << ADCchannels[event.info] << ": " << event.value;
                        break;
                    case SubEventProtection::SUB_EVENT_TYPE_PROTECTION_BAD_SYNC:
                        message_stream << " - Synchronisation failed (The grid frequency is unstable?) ";
                        break;
                    case SubEventProtection::SUB_EVENT_TYPE_PROTECTION_IGBT:
                        message_stream << " - IGBT fault ";
                        message_stream << event.info;
                        break;
                    default:
                        message_stream << stringWithColor(" - Unknown event! ", DARK_RED);
                        break;
                }
                break;
            case EventType::EVENT_TYPE_EVENT:
                message_stream << " - Event ";
                break;
        }

        message(MESSAGE_TYPE_STATE, MESSAGE_NORMAL, MESSAGE_TARGET_ALL,
                message_stream.str());
    }
}

void MainWindow::message(uint8_t type, uint8_t level, uint8_t target, std::string message)
{
    std::string prefix;
    QColor color = Qt::black;
    switch (type)
    {
        case MESSAGE_TYPE_GENERAL:
            prefix = stringWithColor("[GENERAL] ", DARK_GREY);
            break;
        case MESSAGE_TYPE_CONNECTION:
            prefix = stringWithColor("[CONNECTION] ", EXTRA_LIGHT_GREY);
            break;
        case MESSAGE_TYPE_GLOBALFAULT:
            prefix = stringWithColor("[FAULT] ", DARK_RED);
            break;
        case MESSAGE_TYPE_GLOBALWARNING:
            prefix = stringWithColor("[WARNING] ", DARK_RED);
            break;
        case MESSAGE_TYPE_STATE:
            prefix = stringWithColor("[STATE] ", DARK_GREEN);
            break;
        case MESSAGE_TYPE_CAPACITORS:
            prefix = stringWithColor("[CAPACITORS] ", DARK_GREEN);
            break;
        case MESSAGE_TYPE_NETWORK:
            prefix = stringWithColor("[NETWORK] ", DARK_GREEN);
            break;
        default:
            break;
    }
    switch (level)
    {
        case MESSAGE_NORMAL:
            break;
        case MESSAGE_WARNING:
            color = Qt::darkYellow;
            break;
        case MESSAGE_ERROR:
            color = Qt::darkRed;
            break;
    }
    prefix.append(message);
    QTextDocument doc;
    doc.setHtml(QString::fromStdString(prefix));
    QString ss(doc.toPlainText());
    if (target & MESSAGE_TARGET_DEBUG)
    {
        qDebug() << ss;
    }
    if (target & MESSAGE_TARGET_STATUS)
    {
        _ui->statusBar->showMessage(ss, 5000);
    }
    if (target & MESSAGE_TARGET_HISTORY)
    {
        std::string s("<font color=" + LIGHT_GREY + ">");
        s.append(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:").toStdString());
        s.append("</font>");
        s += prefix;
        QListWidgetItem *i=new QListWidgetItem(QString::fromStdString(s), _ui->listLog);
        (void)i;
    }
}

MainWindow::~MainWindow(void)
{
    delete _port_settings;
    delete _ui;
}
void MainWindow::openSerialPort(void)
{
    SettingsDialog::Settings p = _port_settings->settings();

    /* Connect to the default port */
    _pfc->interfaceConnectTo(p.name, p.baudRate, p.dataBits, p.parity,
                            p.stopBits, p.flowControl, p.localEchoEnabled);
}

void MainWindow::closeSerialPort(void)
{
    _pfc->interfaceDisconnect();
}
void MainWindow::about(void)
{
    QMessageBox::about(this, tr("Program info"),
                       tr("The <b>GUI Terminal</b> is used to control the power factor corrector (PFC). Can be connected via a serial port (RS-485 interface) to the PFC. \n"
                          "Author: Stanislav Karpikov [stankarpikov@gmail.com]"));
}

void MainWindow::initInterfaceConnections(void)
{
    connect(_ui->actionConnect,  &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(_ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(_ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(_ui->actionConfigure, &QAction::triggered, _port_settings, &SettingsDialog::show);
    connect(_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(_ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(_port_settings, &SettingsDialog::Appl, this, &MainWindow::openSerialPort);


    connect(_ui->valueCapacitorsKp, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::capacitorsKpValueChanged);
    connect(_ui->valueCapacitorsKi, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::capacitorsKiValueChanged);
    connect(_ui->valueCapacitorsKd, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::capacitorsKdValueChanged);
    connect(_ui->spinCapNominal,    static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::capacitorsNominalValueChanged);
    connect(_ui->spinCapPrecharge,  static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &MainWindow::capacitorsPrechargeValueChanged);
    connect(_ui->buttonStop, &QPushButton::clicked, this, &MainWindow::stopClicked);
    connect(_ui->buttonStart, &QPushButton::clicked, this, &MainWindow::startClicked);
    connect(_ui->buttonSave, &QPushButton::clicked, this, &MainWindow::saveClicked);
    connect(_ui->buttonClearLog, &QPushButton::clicked, this, &MainWindow::actionClearTriggered);
    connect(_ui->checkBoxChannelA, &QCheckBox::toggled, this, &MainWindow::channelACheckToggled);
    connect(_ui->checkBoxChannelB, &QCheckBox::toggled, this, &MainWindow::channelBCheckToggled);
    connect(_ui->checkBoxChannelC, &QCheckBox::toggled, this, &MainWindow::channelCCheckToggled);
    connect(_ui->buttonChargeOn, &QPushButton::clicked, this, &MainWindow::chargeOnClicked);
    connect(_ui->buttonChargeOff, &QPushButton::clicked, this, &MainWindow::chargeOffClicked);
}

void MainWindow::setNetVoltage(float ADC_UD,
                               float ADC_U_A,
                               float ADC_U_B,
                               float ADC_U_C,
                               float ADC_I_A,
                               float ADC_I_B,
                               float ADC_I_C,
                               float ADC_I_ET,
                               float ADC_I_TEMP1,
                               float ADC_I_TEMP2,
                               float ADC_EMS_A,
                               float ADC_EMS_B,
                               float ADC_EMS_C,
                               float ADC_EMS_I,
                               float ADC_MATH_A,
                               float ADC_MATH_B,
                               float ADC_MATH_C)
{
    Q_UNUSED(ADC_I_ET)
    Q_UNUSED(ADC_EMS_A)
    Q_UNUSED(ADC_EMS_B)
    Q_UNUSED(ADC_EMS_C)
    Q_UNUSED(ADC_EMS_I)
    filterApply(_pfc_settings->adc.ADC_U_A, ADC_U_A);
    filterApply(_pfc_settings->adc.ADC_U_B, ADC_U_B);
    filterApply(_pfc_settings->adc.ADC_U_C, ADC_U_C);

    filterApply(_pfc_settings->adc.ADC_I_A, ADC_I_A);
    filterApply(_pfc_settings->adc.ADC_I_B, ADC_I_B);
    filterApply(_pfc_settings->adc.ADC_I_C, ADC_I_C);

    filterApply(_pfc_settings->adc.ADC_MATH_A, ADC_MATH_A);
    filterApply(_pfc_settings->adc.ADC_MATH_B, ADC_MATH_B);
    filterApply(_pfc_settings->adc.ADC_MATH_C, ADC_MATH_C);

    _pfc_settings->adc.ADC_UD = ADC_UD;

    filterApply(_pfc_settings->adc.ADC_I_TEMP1, ADC_I_TEMP1);
    filterApply(_pfc_settings->adc.ADC_I_TEMP2, ADC_I_TEMP2);

    _ui->valueVoltageA->setText(QString().sprintf("% 5.0f V", static_cast<double>(_pfc_settings->adc.ADC_MATH_A)));
    _ui->valueVoltageB->setText(QString().sprintf("% 5.0f V", static_cast<double>(_pfc_settings->adc.ADC_MATH_B)));
    _ui->valueVoltageC->setText(QString().sprintf("% 5.0f V", static_cast<double>(_pfc_settings->adc.ADC_MATH_C)));

    _ui->valueInstantCurrentA->setText(QString().sprintf("% 5.1f A", static_cast<double>(_pfc_settings->adc.ADC_I_A)));
    _ui->valueInstantCurrentB->setText(QString().sprintf("% 5.1f A", static_cast<double>(_pfc_settings->adc.ADC_I_B)));
    _ui->valueInstantCurrentC->setText(QString().sprintf("% 5.1f A", static_cast<double>(_pfc_settings->adc.ADC_I_C)));

    _ui->valueTemperature1->setText(QString().sprintf("% 3.0f °C", static_cast<double>(_pfc_settings->adc.ADC_I_TEMP1)));
    _ui->valueTemperature2->setText(QString().sprintf("% 3.0f °C", static_cast<double>(_pfc_settings->adc.ADC_I_TEMP2)));
}

void MainWindow::setNetVoltageRAW(float ADC_UD,
                                  float ADC_U_A,
                                  float ADC_U_B,
                                  float ADC_U_C,
                                  float ADC_I_A,
                                  float ADC_I_B,
                                  float ADC_I_C,
                                  float ADC_I_ET,
                                  float ADC_I_TEMP1,
                                  float ADC_I_TEMP2,
                                  float ADC_EMS_A,
                                  float ADC_EMS_B,
                                  float ADC_EMS_C,
                                  float ADC_EMS_I)
{
    Q_UNUSED(ADC_I_ET)
    Q_UNUSED(ADC_EMS_A)
    Q_UNUSED(ADC_EMS_B)
    Q_UNUSED(ADC_EMS_C)
    Q_UNUSED(ADC_EMS_I)
    filterApply(_pfc_settings->adc_raw.ADC_U_A, ADC_U_A);
    filterApply(_pfc_settings->adc_raw.ADC_U_B, ADC_U_B);
    filterApply(_pfc_settings->adc_raw.ADC_U_C, ADC_U_C);

    filterApply(_pfc_settings->adc_raw.ADC_I_A, ADC_I_A);
    filterApply(_pfc_settings->adc_raw.ADC_I_B, ADC_I_B);
    filterApply(_pfc_settings->adc_raw.ADC_I_C, ADC_I_C);

    filterApply(_pfc_settings->adc_raw.ADC_UD, ADC_UD);

    filterApply(_pfc_settings->adc_raw.ADC_I_TEMP1, ADC_I_TEMP1);
    filterApply(_pfc_settings->adc_raw.ADC_I_TEMP2, ADC_I_TEMP2);
}

void MainWindow::setNetParams(
    float period_fact,
    float U0Hz_A,
    float U0Hz_B,
    float U0Hz_C,
    float I0Hz_A,
    float I0Hz_B,
    float I0Hz_C,
    float thdu_A,
    float thdu_B,
    float thdu_C,
    float U_phase_A,
    float U_phase_B,
    float U_phase_C)
{
    _pfc_settings->net_params.period_fact = period_fact;

    _pfc_settings->net_params.U0Hz_A = U0Hz_A;
    _pfc_settings->net_params.U0Hz_B = U0Hz_B;
    _pfc_settings->net_params.U0Hz_C = U0Hz_C;
    _pfc_settings->net_params.I0Hz_A = I0Hz_A;
    _pfc_settings->net_params.I0Hz_B = I0Hz_B;
    _pfc_settings->net_params.I0Hz_C = I0Hz_C;

    _pfc_settings->net_params.thdu_A = thdu_A;
    _pfc_settings->net_params.thdu_B = thdu_B;
    _pfc_settings->net_params.thdu_C = thdu_C;

    _pfc_settings->net_params.U_phase_A = U_phase_A * 360.0f / 3.1416f;
    _pfc_settings->net_params.U_phase_B = U_phase_B * 360.0f / 3.1416f;
    _pfc_settings->net_params.U_phase_C = U_phase_C * 360.0f / 3.1416f;

    _ui->valuePhaseAB->setText(QString().sprintf("% 5.1f°", static_cast<double>(_pfc_settings->net_params.U_phase_B)));
    _ui->valuePhaseBC->setText(QString().sprintf("% 5.1f°", static_cast<double>(_pfc_settings->net_params.U_phase_C)));

    _ui->valueTHDUA->setText(QString().sprintf("% 5.2f°", static_cast<double>(_pfc_settings->net_params.thdu_A)));
    _ui->valueTHDUB->setText(QString().sprintf("% 5.2f°", static_cast<double>(_pfc_settings->net_params.thdu_B)));
    _ui->valueTHDUC->setText(QString().sprintf("% 5.2f°", static_cast<double>(_pfc_settings->net_params.thdu_C)));

    _ui->valueFrequency->setText(QString().sprintf("% 6.3f Hz", static_cast<double>(1.0f / (_pfc_settings->net_params.period_fact / 1000000.0f))));
}

void MainWindow::ansSettingsCalibrations(bool writed)
{
    Q_UNUSED(writed)
}

void MainWindow::ansSettingsProtection(bool writed)
{
    Q_UNUSED(writed)
}

void MainWindow::ansSettingsCapacitors(bool writed)
{
    Q_UNUSED(writed)
}

void MainWindow::stopClicked(void)
{
    emit writeSwitchOnOff(PFCCommands::COMMAND_WORK_OFF, 0);
}
void MainWindow::startClicked(void)
{
    emit writeSwitchOnOff(PFCCommands::COMMAND_WORK_ON, 0);
}
void MainWindow::setSwitchOnOff(uint32_t result)
{
    Q_UNUSED(result)
}
void MainWindow::saveClicked(void)
{
    writeSwitchOnOff(PFCCommands::COMMAND_SETTINGS_SAVE, 0);
}
void MainWindow::actionClearTriggered(void)
{
    _ui->listLog->clear();
}

void MainWindow::channelACheckToggled(bool checked)
{
    emit writeSwitchOnOff(PFCCommands::COMMAND_CHANNEL0_DATA, checked);
}

void MainWindow::channelBCheckToggled(bool checked)
{
    emit writeSwitchOnOff(PFCCommands::COMMAND_CHANNEL1_DATA, checked);
}

void MainWindow::channelCCheckToggled(bool checked)
{
    emit writeSwitchOnOff(PFCCommands::COMMAND_CHANNEL2_DATA, checked);
}

void MainWindow::chargeOnClicked(void)
{
    emit writeSwitchOnOff(PFCCommands::COMMAND_CHARGE_ON, 0);
}

void MainWindow::chargeOffClicked(void)
{
    emit writeSwitchOnOff(PFCCommands::COMMAND_CHARGE_OFF, 0);
}
