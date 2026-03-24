#include "WebPortal.h"

WebPortal *WebPortal::_self = nullptr;

WebPortal::WebPortal(fs::FS &fs,
                     const char *ssid,
                     const char *password,
                     const char *otaLogin,
                     const char *otaPassword,
                     DataProvider dataProvider,
                     ApplyConfigCallback applyConfig,
                     SavePagesCallback savePages,
                     const String &versionText)
    : _fs(fs),
      _ui(),
      _ssid(ssid),
      _password(password),
      _otaLogin(otaLogin),
      _otaPassword(otaPassword),
      _versionText(versionText),
      _dataProvider(dataProvider),
      _applyConfig(applyConfig),
      _savePages(savePages)
{
    _self = this;
}

void WebPortal::begin()
{
    beginWifi();

    _ui.uploadAuto(0);
    _ui.deleteAuto(0);
    _ui.downloadAuto(0);
    _ui.renameAuto(0);

    _ui.attachBuild(buildPageThunk);
    _ui.attach(actionPageThunk);

    _ui.start();
    _ui.enableOTA(_otaLogin, _otaPassword);
}

void WebPortal::tick()
{
    _ui.tick();
}

void WebPortal::beginWifi()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(_ssid, _password, 1, 0, 2);
}

void WebPortal::buildPageThunk()
{
    if (_self)
        _self->buildPage();
}

void WebPortal::actionPageThunk()
{
    if (_self)
        _self->actionPage();
}

void WebPortal::buildPage()
{
    WebPortalData d;
    if (_dataProvider)
        d = _dataProvider();

    GP.BUILD_BEGIN(1024);
    GP.THEME(GP_DARK);
    // GP.GRID_RESPONSIVE(800);
    GP.UPDATE("style,canSpeed,engineLoad,vehicleType,tankVolume,distance,rpm,oilFuelRate,wheelSpeed,vehicleWeight,airTemper,engineTemper,oilTemper,fuelTemper,exhaustGasTemper,state,acc,cngInjectionTime,cngIstValue,cngRailPressure,cngTurboPressure,cngLevel,cngWaterTemperature,cngRailTemperature,cngTripFuel,cngTotalFuelUsed,cngDieselReduction,error,busErrCounter");
    GP.TITLE("Дисплей EVO | " + _versionText, "");
    GP.NAV_TABS("Телеметрия,Настройка");

    GP.NAV_BLOCK_BEGIN();
    M_BOX(GP.LABEL("Экономия дизельного топлива, %", "", GP_GRAY_B, 0, 1); GP.LABEL("-", "style", GP_GRAY_B, 0, 1););
    M_GRID(
        M_BLOCK_TAB(
            "АВТОМОБИЛЬ", "400", GP_GRAY_B,
            M_BOX(GP.LABEL("Пробег, км", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "distance"););
            M_BOX(GP.LABEL("Обороты RPM, об/мин", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "rpm"););
            M_BOX(GP.LABEL("Мг. расход ДТ, л/час", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "oilFuelRate"););
            M_BOX(GP.LABEL("Скорость, км/ч", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "wheelSpeed"););
            M_BOX(GP.LABEL("Нагрузка двигатель, %", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "engineLoad"););
            M_BOX(GP.LABEL("Вес, т", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "vehicleWeight"););
            M_BOX(GP.LABEL("Т воздух, °C", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "airTemper"););
            M_BOX(GP.LABEL("Т двигатель, °C", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "engineTemper"););
            M_BOX(GP.LABEL("Т масло, °C", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "oilTemper"););
            M_BOX(GP.LABEL("T топливо, °C", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "fuelTemper"););
            M_BOX(GP.LABEL("Т выхлоп, °C", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "exhaustGasTemper");););

        M_BLOCK_TAB(
            "EVO NEW", "400", GP_ORANGE_B,
            M_BOX(GP.LABEL("Режим", "", GP_GRAY_B, 0, 1); GP.LABEL("-", "state", GP_GRAY_B, 0, 1););
            M_BOX(GP.LABEL("Педаль, %", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "acc"););
            M_BOX(GP.LABEL("Газ, мс", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngInjectionTime"););
            M_BOX(GP.LABEL("Газ, кг/ч", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngIstValue"););
            M_BOX(GP.LABEL("Карта ДТ", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngDieselReduction"););
            M_BOX(GP.LABEL("Турбо, бар", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngTurboPressure"););
            M_BOX(GP.LABEL("Уровень, бар", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngLevel"););
            M_BOX(GP.LABEL("Редуктор, бар", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngRailPressure"););
            M_BOX(GP.LABEL("T ред °C", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngWaterTemperature"););
            M_BOX(GP.LABEL("T газ °C", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngRailTemperature"););
            M_BOX(GP.LABEL("Расход газ, кг", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngTripFuel"););
            M_BOX(GP.LABEL("Расход итого, кг", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngTotalFuelUsed"););
            M_BOX(GP.LABEL("Счетчик кан ошибок", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "busErrCounter"););
            M_BOX(GP.LABEL("Флаги EVO", "", GP_GRAY, 0, 1););
            M_BOX(GP.LABEL("-", "error", GP_RED_B, 0, 0););););
    GP.NAV_BLOCK_END();

    GP.NAV_BLOCK_BEGIN();
    M_GRID(
        M_BLOCK_TAB("", M_BOX(GP.LABEL("Скорость can шины, кБ"); GP.LABEL(String(d.canSpeed), "canSpeed");); M_BOX(GP.LABEL("Тип данных в can"); GP.LABEL(d.vehicleType, "vehicleType");); M_BOX(GP.LABEL("Объем газовых баллонов, л"); GP.LABEL(String(d.tankVolume), "tankVolume");); GP.FILE_UPLOAD("file_upl", "Загрузить файл настроек", "", GP_ORANGE_B);
                    // GP.FILE_MANAGER(&_fs);
                    GP.FORM_BEGIN("/save_pages");

                    // Добавляем форму для каждой страницы
                    for (int i = 0; i < WebPortalData::WEB_PAGE_COUNT; ++i) {
                        const auto &page = d.pageCfg[i];

                        if (!page.systemPage) {
                            String enId = "pg_en_" + String(i);
                            String ordId = "pg_ord_" + String(i);
                            GP.CHECK(enId.c_str(), page.enabled); GP.LABEL(page.title); //GP.NUMBER(ordId.c_str(), "Порядок", page.order, "20", true);
    
                        } 
                    }
                    GP.SUBMIT("save", "Сохранить страницы", GP_GREEN);););
    GP.FORM_END();
    GP.NAV_BLOCK_END();

    GP.FORM_END();
    GP.BUILD_END();
}

void WebPortal::actionPage()
{
    if (_ui.update())
    {
        WebPortalData d;
        if (_dataProvider)
            d = _dataProvider();

        _ui.updateInt("canSpeed", d.canSpeed);
        _ui.updateString("vehicleType", d.vehicleType);
        _ui.updateInt("tankVolume", d.tankVolume);
        _ui.updateInt("busErrCounter", d.busErrCounter);

        if (d.evoOk)
        {
            String str = d.state ? "Газ" : "Дт";
            _ui.updateInt("style", d.levelEconomicalDriving);
            _ui.updateString("state", str);
            _ui.updateInt("acc", d.ACC);
            _ui.updateInt("cngLevel", d.cngLevel);
            _ui.updateInt("cngRailTemperature", d.cngRailTemperature);
            _ui.updateInt("cngWaterTemperature", d.cngWaterTemperature);
            _ui.updateInt("cngDieselReduction", d.cngDieselReduction);
            _ui.updateFloat("cngTurboPressure", d.cngTurboPressure, 1);
            _ui.updateFloat("cngRailPressure", d.cngRailPressure, 1);
            _ui.updateFloat("cngTripFuel", d.cngTripFuel, 1);
            _ui.updateFloat("cngTotalFuelUsed", d.cngTotalFuelUsed, 1);
            _ui.updateFloat("cngInjectionTime", d.cngInjectionTime, 1);
            _ui.updateFloat("cngIstValue", d.cngIstValue, 1);
            _ui.updateString("error", d.error);
        }
        else
        {
            String str = "no can EVO";
            _ui.updateInt("style", 0);
            _ui.updateString("state", str);
            _ui.updateInt("acc", 0);
            _ui.updateInt("cngLevel", 0);
            _ui.updateInt("cngRailTemperature", 0);
            _ui.updateInt("cngWaterTemperature", 0);
            _ui.updateInt("cngDieselReduction", 0);
            _ui.updateInt("cngTurboPressure", 0);
            _ui.updateInt("cngRailPressure", 0);
            _ui.updateInt("cngTripFuel", 0);
            _ui.updateInt("cngTotalFuelUsed", 0);
            _ui.updateInt("cngInjectionTime", 0);
            _ui.updateInt("cngIstValue", 0);
            _ui.updateString("error", str);
        }
        {
            // float dt = d.distance / 1000.0;
            _ui.updateFloat("distance", d.distance);
            _ui.updateInt("rpm", d.rpm);
            _ui.updateFloat("oilFuelRate", d.oilFuelRate, 1);
            _ui.updateFloat("wheelSpeed", d.wheelSpeed, 1);
            _ui.updateInt("engineLoad", d.engineLoad);
            _ui.updateFloat("vehicleWeight", d.vehicleWeight, 1);
            _ui.updateInt("airTemper", d.airTemper);
            _ui.updateInt("engineTemper", d.engineTemper);
            _ui.updateInt("oilTemper", d.oilTemper);
            _ui.updateInt("fuelTemper", d.fuelTemper);
            _ui.updateInt("exhaustGasTemper", d.exhaustGasTemper);
        }

        if (_ui.upload())
        {
            File file = _fs.open("/" + _ui.fileName(), "w");
            if (file)
            {
                _ui.saveFile(file);
                file.close();

                if (_applyConfig)
                    _applyConfig(_ui.fileName());
            }
        }

        if (_ui.download())
            _ui.sendFile(_fs.open(_ui.uri(), "r"));

        if (_ui.deleteFile())
            _fs.remove(_ui.deletePath());

        // if (_ui.renameFile())
        //     _fs.rename(_ui.renamePath(), _ui.renameName());
    }
    if (_ui.form("/save_pages") || _ui.click("save"))
    {
        Serial.println("Save");

        WebPageCfg pages[WebPortalData::WEB_PAGE_COUNT];

        WebPortalData d;
        if (_dataProvider)
            d = _dataProvider();

        for (int i = 0; i < WebPortalData::WEB_PAGE_COUNT; ++i)
        {
            pages[i] = d.pageCfg[i];
        }

        pages[0].enabled = _ui.getBool("pg_en_0");
        pages[0].order = _ui.getInt("pg_ord_0");

        pages[1].enabled = _ui.getBool("pg_en_1");
        pages[1].order = _ui.getInt("pg_ord_1");

        pages[2].enabled = _ui.getBool("pg_en_2");
        pages[2].order = _ui.getInt("pg_ord_2");

        pages[3].enabled = _ui.getBool("pg_en_3");
        pages[3].order = _ui.getInt("pg_ord_3");

        pages[4].enabled = _ui.getBool("pg_en_4");
        pages[4].order = _ui.getInt("pg_ord_4");

        if (_savePages)
            _savePages(pages, WebPortalData::WEB_PAGE_COUNT);
    }
}

void WebPortal::updateDisplayPageConfig(int index, bool enabled, int order)
{
    if (!_dataProvider)
        return;

    WebPortalData d = _dataProvider();

    if (index < 0 || index >= WebPortalData::WEB_PAGE_COUNT)
        return;

    if (d.pageCfg[index].systemPage)
        return;

    // Здесь WebPortal только читает значения из формы.
    // Фактическое применение должно делать внешнее хранилище/колбэк через provider.
    // Поэтому этот метод оставлен как точка расширения.
}

void WebPortal::normalizeDisplayPageOrder()
{
    // Нормализация делается во внешнем слое конфигурации.
    // Метод оставлен как точка расширения для дальнейшего переноса логики внутрь WebPortal.
}