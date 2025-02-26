/**
 * @file mainwindow.h
 * @author Stanislav Karpikov
 * @brief Main application window management
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/** @addtogroup app_main
 * @{
 */

/*--------------------------------------------------------------
                       INCLUDES
--------------------------------------------------------------*/

#include <QtCore/QtGlobal>
#include <QDebug>
#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QTimer>
#include <device.h>
#include <QThread>
#include <QSignalSpy>
#include "page_filters.h"
#include "page_oscillog.h"
#include "page_main.h"
#include "page_settingscalibrations.h"
#include "page_settingsprotection.h"
#include "page_settingscapacitors.h"
#include "qcustomplot.h"
#include "device_definition.h"

/*--------------------------------------------------------------
                       PUBLIC TYPES
--------------------------------------------------------------*/

QT_BEGIN_NAMESPACE

namespace Ui {
    class MainWindow;
}

QT_END_NAMESPACE

class SettingsDialog;
class PFC;

/*--------------------------------------------------------------
                       CLASSES
--------------------------------------------------------------*/


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = Q_NULLPTR);
    ~MainWindow(void);

    /*--------------------------------------------------------------
                           PRIVATE DATA
    --------------------------------------------------------------*/
private:
    static constexpr auto FCOEFF = 0.9f;

    static constexpr auto TIMEOUT_UPDATE_MAIN_PARAMS = static_cast<std::chrono::milliseconds>(300);
    static constexpr auto TIMEOUT_UPDATE_VOLTAGES = static_cast<std::chrono::milliseconds>(300);
    static constexpr auto TIMEOUT_UPDATE_ADC_RAW = static_cast<std::chrono::milliseconds>(300);
    static constexpr auto TIMEOUT_UPDATE_STATE = static_cast<std::chrono::milliseconds>(300);
    static constexpr auto TIMEOUT_UPDATE_VERSION = static_cast<std::chrono::milliseconds>(3000);
    static constexpr auto TIMEOUT_UPDATE_OSCILLOG = static_cast<std::chrono::milliseconds>(54);
    static constexpr auto TIMEOUT_UPDATE_SETTINGS_CALIBRATIONS = static_cast<std::chrono::milliseconds>(300);
    static constexpr auto TIMEOUT_UPDATE_SETTINGS_CAPACITORS = static_cast<std::chrono::milliseconds>(300);
    static constexpr auto TIMEOUT_UPDATE_SETTINGS_PROTECTION = static_cast<std::chrono::milliseconds>(300);
    static constexpr auto TIMEOUT_UPDATE_SETTINGS_FILTERS = static_cast<std::chrono::milliseconds>(300);

    const static auto EVENTS_TIMER_TIMEOUT = 1000;    
    const static auto UD_MAX_VALUE = 500;



    enum class TableProtectionRows
    {
        ROW_UD_MIN,
        ROW_UD_MAX,
        ROW_TEMPERATURE,
        ROW_U_MIN,
        ROW_U_MAX,
        ROW_F_MIN,
        ROW_F_MAX,
        ROW_I_MAX_RMS,
        ROW_I_MAX_PEAK
    };


    enum class TableCalibrationRows
    {
            table_calibrations_row_offset_U_cap,
            table_calibrations_row_offset_U_A,
            table_calibrations_row_offset_U_B,
            table_calibrations_row_offset_U_C,
            table_calibrations_row_offset_I_A,
            table_calibrations_row_offset_I_B,
            table_calibrations_row_offset_I_C,
            table_calibrations_row_offset_I_et,
            table_calibrations_row_offset_temperature_1,
            table_calibrations_row_offset_temperature_2,
            table_calibrations_row_offset_U_EMS_A,
            table_calibrations_row_offset_U_EMS_B,
            table_calibrations_row_offset_U_EMS_C,
            table_calibrations_row_offset_U_EMS_I,
            table_calibrations_row_multiplier_U_cap,
            table_calibrations_row_multiplier_U_A,
            table_calibrations_row_multiplier_U_B,
            table_calibrations_row_multiplier_U_C,
            table_calibrations_row_multiplier_I_A,
            table_calibrations_row_multiplier_I_B,
            table_calibrations_row_multiplier_I_C,
            table_calibrations_row_multiplier_I_et,
            table_calibrations_row_multiplier_temperature_1,
            table_calibrations_row_multiplier_temperature_2,
            table_calibrations_row_multiplier_U_EMS_A,
            table_calibrations_row_multiplier_U_EMS_B,
            table_calibrations_row_multiplier_U_EMS_C,
            table_calibrations_row_multiplier_U_EMS_I,
            table_calibrations_row_count
    };

    enum class TableCalibrationColumns
    {
        TABLE_CALIBRATIONS_VALUE_COLUMN,
        TABLE_CALIBRATIONS_AUTO_BUTTON_COLUMN,
        TABLE_CALIBRATIONS_AUTO_VALUE_COLUMN
    };

    friend MainWindow::TableCalibrationRows& operator++(MainWindow::TableCalibrationRows& row, int);

    /* Data */
    Ui::MainWindow *_ui;
    PFC *_pfc; /*TODO: Replace by a smart pointer */
    PFCconfig::PFCsettings *_pfc_settings; /*TODO: Replace by a smart pointer */
    PageFilters _page_filters;
    PageOscillog _page_oscillog;
    PageMain _page_main;

    std::list<QPushButton*> _buttons_edit;
    uint64_t _last_index_events;

    SettingsDialog *_port_settings;
    QTimer _timer_main_params;
    QTimer _timer_raw;
    QTimer _timer_state;
    QTimer _timer_voltage;
    QTimer _timer_version;
    QTimer _timer_oscillog;
    QTimer _timer_events;
    QTimer _timer_settings_calibrations;
    QTimer _timer_settings_capacitors;
    QTimer _timer_settings_protection;
    QTimer _timer_settings_filters;
    bool _connected;

    std::vector<QPushButton*> _btns_edit;

    /*--------------------------------------------------------------
                           PRIVATE FUNCTIONS
    --------------------------------------------------------------*/
private:
    bool eventFilter(QObject *object, QEvent *event);

    void pageSettingsCalibrationsInit(void);
    void pageSettingsCapacitorsInit(void);
    void pageSettingsProtectionInit(void);
    void setTableProtectionsVal(TableProtectionRows row, float value);
    void updateSpinVal(QDoubleSpinBox *spinbox, float value);
    void initInterfaceConnections(void);
    void filterApply(float &A, float B);
    void setFilter(QEvent* event, QObject* object, QWidget* ui_obj, QTimer* obj, std::chrono::milliseconds timeout);
    std::string stringWithColor(std::string str, std::string color);
    void tableSettingsCalibrationsSetAutoSettings(void);
    float CALC_AUTO_COEF(PFCconfig::ADC::ADCchannel CALIB, float NOW, float NOMINAL);

private slots:
    /* Interface slots */

    void capacitorsKpValueChanged(double arg);
    void capacitorsKiValueChanged(double arg);
    void capacitorsKdValueChanged(double arg);
    void capacitorsNominalValueChanged(double arg);
    void capacitorsPrechargeValueChanged(double arg);
    void stopClicked(void);
    void startClicked(void);
    void saveClicked(void);
    void actionClearTriggered(void);
    void channelACheckToggled(bool checked);
    void channelBCheckToggled(bool checked);
    void channelCCheckToggled(bool checked);
    void chargeOnClicked(void);
    void chargeOffClicked(void);

    /* Internal slots */
    void tableSettingsCalibrationsAutoClicked(bool check);
    /*--------------------------------------------------------------
                           PUBLIC FUNCTIONS
    --------------------------------------------------------------*/
public slots:
    void openSerialPort(void);
    void closeSerialPort(void);
    void about(void);
    void message(uint8_t type, uint8_t level, uint8_t target, std::string message);
    void deviceConnected(void);
    void deviceDisconnected(void);
    void setConnection(bool connected);
    /* Interface commands */

    void setNetVoltage( float ADC_UD,
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
                        float ADC_MATH_C);
    void setNetVoltageRAW( float ADC_UD,
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
                           float ADC_EMS_I);

    void setSwitchOnOff(uint32_t result);
    void setEvents(std::list<PFCconfig::Events::EventRecord> ev);


    void setNetParams(float period_fact,
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
                    float U_phase_C);
    void setSettingsCalibrations(
            std::vector<float> calibration,
            std::vector<float> offset
            );

    void setSettingsProtection(
            float Ud_min,
            float Ud_max,
            float temperature,
            float U_min,
            float U_max,
            float Fnet_min,
            float Fnet_max,
            float I_max_rms,
            float I_max_peak
            );
    void setSettingsCapacitors(
            float ctrlUd_Kp,
            float ctrlUd_Ki,
            float ctrlUd_Kd,
            float Ud_nominal,
            float Ud_precharge);
    void ansSettingsCalibrations(bool writed);
    void ansSettingsProtection(bool writed);
    void ansSettingsCapacitors(bool writed);

    /* Timer events callbacks */
    void timerOscillog();
    void timerNetParams();
    void timerWorkState();
    void timerVersion();
    void timerSettingsCalibrations();
    void timerSettingsCapacitors();
    void timerSettingsProtection();
    void timerSettingsFilters();
    void timerupdateNetVoltage();
    void timerupdateNetVoltageRaw();
    void timerEvents();

    /* Other functions */

    void tableSettingsCalibrationsChanged(int row, int col);
    void tableSettingsProtectionChanged(int row, int col);

signals:
    void updateNetVoltage();
    void updateNetVoltageRAW();
    void updateNetParams();
    void updateEvents(uint64_t afterIndex);
    void updateSettingsCalibrations();
    void updateSettingsProtection();
    void updateSettingsCapacitors();

    void writeSettingsCalibrations(
            std::vector<float> calibration,
            std::vector<float> offset);
    void writeSettingsProtection(
            float Ud_min,
            float Ud_max,
            float temperature,
            float U_min,
            float U_max,
            float Fnet_min,
            float Fnet_max,
            float I_max_rms,
            float I_max_peak);
    void writeSettingsCapacitors(
            float ctrlUd_Kp,
            float ctrlUd_Ki,
            float ctrlUd_Kd,
            float Ud_nominal,
            float Ud_precharge);
    void writeSwitchOnOff(PFCconfig::Interface::PFCCommands command,uint32_t data);
};

/** @} */

#endif /* MAINWINDOW_H */
