#include "config.hpp"

#include "config_alarm.hpp"
#include "serializer.hpp"
#include "toolbox_filesystem.hpp"

#ifndef ALARM_ASSETS_DIR
#define ALARM_ASSETS_DIR "/usr/share/alarm"
#endif

namespace
{
constexpr char kKeyAlsaDevice[] = "alsa_device";
constexpr char kKeyAssetsFolder[] = "assets_folder";
constexpr char kKeyDisplayDriver[] = "display_driver";
constexpr char kKeyDisplayWidth[] = "display_width";
constexpr char kKeyDisplayHeight[] = "display_height";
constexpr char kKeyDisplaySeconds[] = "display_seconds";
constexpr char kKeyFramesPerSecond[] = "frames_per_second";
constexpr char kKeySensorThermal[] = "sensor_thermal";
constexpr char kKeyHandClockColor[] = "hand_clock_color";
constexpr char kKeyAlarms[] = "alarms";

std::string getAssetFile(std::string_view assetFolder, std::string_view assetType, std::string_view assetFile)
{
    fs::path path{assetFolder};
    path /= assetType;
    if (assetFile.empty() == false)
    {
        path /= assetFile;
    }
    return path.native();
}

} // namespace

struct Config::Impl
{
    bool displaySeconds = true;
    std::string assetsFolder = ALARM_ASSETS_DIR;
    std::string alsaDevice = "default";
    std::string displayDriver;
    int displayWidth = 320;
    int displayHeight = 240;
    int framesPerSecond = 25;
    std::string temperatureSensor;
    uint8_t clockHandColor[3] = {255, 0, 0};
    std::list<ConfigAlarm> alarms;
};

Config::Config()
    : pimpl(std::make_unique<Impl>())
{
}

Config::~Config() = default;

const char *Config::getAlsaDevice() const
{
    return pimpl->alsaDevice.c_str();
}

void Config::setAlsaDevice(std::string_view device)
{
    pimpl->alsaDevice = device;
}

std::string_view Config::getAssetsFolder() const
{
    return pimpl->assetsFolder;
}

void Config::setAssetsFolder(std::string_view folder)
{
    pimpl->assetsFolder = folder;
}

bool Config::displaySeconds() const
{
    return pimpl->displaySeconds;
}

void Config::setDisplaySeconds(bool d)
{
    pimpl->displaySeconds = d;
}

std::string_view Config::getDisplayDriver() const
{
    return pimpl->displayDriver;
}

void Config::setDisplayDriver(std::string_view driver)
{
    pimpl->displayDriver = driver;
}

int Config::getDisplayWidth() const
{
    return pimpl->displayWidth;
}

void Config::setDisplayWidth(int w)
{
    pimpl->displayWidth = w;
}

int Config::getDisplayHeight() const
{
    return pimpl->displayHeight;
}

void Config::setDisplayHeight(int h)
{
    pimpl->displayHeight = h;
}

const uint8_t (&Config::getClockHandColor() const)[3]
{
    return pimpl->clockHandColor;
}

uint8_t (&Config::getClockHandColor())[3]
{
    return pimpl->clockHandColor;
}

int Config::getFramesPerSecond() const
{
    return pimpl->framesPerSecond;
}

void Config::setFramesPerSecond(int fps)
{
    pimpl->framesPerSecond = fps;
}

std::string_view Config::getSensorThermal() const
{
    return pimpl->temperatureSensor;
}

void Config::setSensorThermal(std::string_view name)
{
    pimpl->temperatureSensor = name;
}

const std::list<ConfigAlarm> &Config::getAlarms() const
{
    return pimpl->alarms;
}

std::list<ConfigAlarm> &Config::getAlarms()
{
    return pimpl->alarms;
}

void Config::load(const Deserializer &deserializer)
{
    if (const auto device = deserializer.getString(kKeyAlsaDevice))
    {
        setAlsaDevice(*device);
    }
    if (const auto assetsFolder = deserializer.getString(kKeyAssetsFolder))
    {
        setAssetsFolder(*assetsFolder);
    }
    if (const auto displayDriver = deserializer.getString(kKeyDisplayDriver))
    {
        setDisplayDriver(*displayDriver);
    }
    if (const auto displayWidth = deserializer.getInt(kKeyDisplayWidth))
    {
        setDisplayWidth(*displayWidth);
    }
    if (const auto displayHeight = deserializer.getInt(kKeyDisplayHeight))
    {
        setDisplayHeight(*displayHeight);
    }
    if (const auto displaySeconds = deserializer.getBool(kKeyDisplaySeconds))
    {
        setDisplaySeconds(*displaySeconds);
    }
    if (const auto fps = deserializer.getInt(kKeyFramesPerSecond))
    {
        setFramesPerSecond(*fps);
    }
    if (const auto temperatureSensor = deserializer.getString(kKeySensorThermal))
    {
        setSensorThermal(*temperatureSensor);
    }

    if (const auto r = deserializer.getIntFromArrayAt(kKeyHandClockColor, 0),
        g = deserializer.getIntFromArrayAt(kKeyHandClockColor, 1),
        b = deserializer.getIntFromArrayAt(kKeyHandClockColor, 2);
        r && g && b)
    {
        auto &rgb = getClockHandColor();
        rgb[0] = *r;
        rgb[1] = *g;
        rgb[2] = *b;
    }

    auto &alarms = getAlarms();
    for (size_t idx = 0; idx < deserializer.getArraySize(kKeyAlarms); ++idx)
    {
        alarms.emplace_back();
        deserializer.getObjectFromArrayAt(kKeyAlarms, idx, alarms.back());
    }
}

void Config::save(Serializer &serializer) const
{
    serializer.setString(kKeyAlsaDevice, getAlsaDevice());
    serializer.setString(kKeyAssetsFolder, getAssetsFolder());
    if (const auto driver = getDisplayDriver(); !driver.empty())
    {
        serializer.setString(kKeyDisplayDriver, driver);
    }
    serializer.setInt(kKeyDisplayWidth, getDisplayWidth());
    serializer.setInt(kKeyDisplayHeight, getDisplayHeight());
    serializer.setBool(kKeyDisplaySeconds, displaySeconds());
    serializer.setInt(kKeyFramesPerSecond, getFramesPerSecond());
    if (const auto name = getSensorThermal(); !name.empty())
    {
        serializer.setString(kKeySensorThermal, name);
    }

    for (const int rgb : getClockHandColor())
    {
        serializer.appendIntToArray(kKeyHandClockColor, rgb);
    }

    for (const auto &alarm : getAlarms())
    {
        serializer.appendObjectToArray(kKeyAlarms, alarm);
    }
}

std::string Config::getMusic(std::string_view filename) const
{
    return getAssetFile(getAssetsFolder(), "music", filename);
}

std::string Config::getShader(std::string_view filename) const
{
    return getAssetFile(getAssetsFolder(), "shader", filename);
}

std::string Config::getTexture(std::string_view filename) const
{
    return getAssetFile(getAssetsFolder(), "textures", filename);
}

std::string Config::getMessages() const
{
    return getAssetFile(getAssetsFolder(), "messages", "");
}
