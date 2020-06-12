#include "screen_set_alarm_file.hpp"

#include "audio.hpp"
#include "config.hpp"
#include "config_alarm.hpp"
#include "context.hpp"
#include "renderer.hpp"
#include "renderer_sprite.hpp"
#include "renderer_text.hpp"
#include "toolbox_filesystem.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <optional>

namespace
{

constexpr size_t kInvalidIdx = -1;
constexpr size_t kFilenameSize = 34;

std::vector<std::string> getFiles(const fs::path &folder)
{
    std::vector<std::string> files;
    for (const auto &file : fs::directory_iterator{folder})
    {
        if (fs::is_regular_file(file))
        {
            files.push_back(file.path().filename().native());
        }
    }
    return files;
}

ConfigAlarm *getAlarm(Context &ctx, size_t alarmIdx)
{
    if (auto &alarms = ctx.getConfig().getAlarms(); alarmIdx < alarms.size())
    {
        return &(*std::next(alarms.begin(), alarmIdx));
    }
    return nullptr;
}

size_t getFilenameIdx(const std::vector<std::string> &filenames,
                      std::string_view filename)
{
    if (const auto it = std::find(filenames.begin(), filenames.end(), filename); it != filenames.end())
    {
        return it - filenames.begin();
    }
    return kInvalidIdx;
}

void advanceAlarm(ConfigAlarm *alarm, const std::vector<std::string> &filenames, int modifier)
{
    if (alarm)
    {
        const size_t idx = getFilenameIdx(filenames, alarm->getFile());
        const size_t newIdx = idx + modifier;
        if (idx < filenames.size() && newIdx < filenames.size())
        {
            std::cerr << "Set alarm to " << filenames[newIdx] << std::endl;
            alarm->setFile(filenames[newIdx]);
        }
    }
}

} // namespace

struct ScreenSetAlarmFile::Impl
{
    Impl(Renderer &renderer, std::string_view musicFolder)
        : arrowUp{renderer.renderSprite(Asset::Arrow, renderer.getWidth() / 2, renderer.getHeight(), Position::Up)},
          arrowDown{renderer.renderSprite(Asset::Arrow, renderer.getWidth() / 2, 0, Position::Down, 2)},
          arrowLeft{renderer.renderSprite(Asset::Arrow, 0, renderer.getHeight() / 2, Position::Left, 3)},
          arrowRight{renderer.renderSprite(Asset::Arrow, renderer.getWidth(), renderer.getHeight() / 2, Position::Right, 1)},
          previousScreen{renderer.renderStaticText(kMargin, renderer.getHeight() - kMargin, "Alarme\nHeure", Position::UpLeft, 16)},
          nextScreen{renderer.renderStaticText(renderer.getWidth() - kMargin, renderer.getHeight() - kMargin, "Config\n  Date", Position::UpRight, 16)},
          errorText{renderer.renderText(renderer.getWidth() / 2, renderer.getHeight() / 2, 14, 1, Position::Center)},
          alarmFilenameText{renderer.renderText(renderer.getWidth() / 2, renderer.getHeight() / 2, kFilenameSize, 1, Position::Up, 16)},
          alarmNumberText{renderer.renderText(renderer.getWidth() / 2, renderer.getHeight() * 3 / 4, 9, 1, Position::Up)},
          filenames{getFiles(musicFolder)}
    {
    }

    void refreshAlarmNumberText(size_t idx)
    {
        if (idx != alarmNumberTextIdx)
        {
            char buffer[32];
            std::sprintf(buffer, "Alarme %d", static_cast<int>(idx));
            alarmNumberText.set(buffer);
            alarmNumberTextIdx = idx;
        }
    }

    void refreshAlarmFilenameText(size_t filenameIdx)
    {
        if (filenameIdx != alarmFilenameTextIdx)
        {
            std::string_view filename = filenames[filenameIdx];
            if (filename.size() > kFilenameSize)
            {
                filename.remove_suffix(filename.size() - kFilenameSize);
            }

            char buffer[2 * kFilenameSize + 1];
            std::memset(buffer, ' ', sizeof(buffer) - 1);
            std::memcpy(buffer + kFilenameSize, filename.data(), filename.size());
            const size_t missingLen = kFilenameSize - filename.size();
            char *ptr = buffer + kFilenameSize - missingLen / 2;
            ptr[kFilenameSize] = '\0';

            alarmFilenameText.set(ptr);
            alarmFilenameTextIdx = filenameIdx;
        }
    }

    RendererSprite arrowUp;
    RendererSprite arrowDown;
    RendererSprite arrowLeft;
    RendererSprite arrowRight;

    RendererTextStatic previousScreen;
    RendererTextStatic nextScreen;

    RendererText errorText;
    RendererText alarmFilenameText;
    RendererText alarmNumberText;

    size_t alarmIdx = 0;
    size_t alarmNumberTextIdx = kInvalidIdx;
    size_t alarmFilenameTextIdx = kInvalidIdx;
    std::vector<std::string> filenames;
};

ScreenSetAlarmFile::ScreenSetAlarmFile(Context &ctx)
    : Screen{ctx}
{
}

ScreenSetAlarmFile::~ScreenSetAlarmFile() = default;

void ScreenSetAlarmFile::enter()
{
    pimpl = std::make_unique<Impl>(ctx.getRenderer(), ctx.getConfig().getMusic());

    if (!pimpl->filenames.empty())
    {
        for (ConfigAlarm &alarm : ctx.getConfig().getAlarms())
        {
            if (size_t idx = getFilenameIdx(pimpl->filenames, alarm.getFile()); idx >= pimpl->filenames.size())
            {
                std::cerr << "Could not find the file \"" << alarm.getFile() << "\" setting it" << std::endl;
                alarm.setFile(pimpl->filenames.front());
            }
        }
    }
}

void ScreenSetAlarmFile::leave()
{
    pimpl = nullptr;
}

void ScreenSetAlarmFile::run(const Clock::time_point &)
{
    pimpl->previousScreen.print();
    pimpl->nextScreen.print();

    if (const auto alarm = getAlarm(ctx, pimpl->alarmIdx))
    {
        pimpl->refreshAlarmNumberText(pimpl->alarmIdx);
        pimpl->alarmNumberText.print();

        if (const size_t filenameIdx = getFilenameIdx(pimpl->filenames, alarm->getFile());
            filenameIdx < pimpl->filenames.size())
        {
            if (filenameIdx + 1 < pimpl->filenames.size())
            {
                pimpl->arrowUp.print();
            }
            if (filenameIdx - 1 < pimpl->filenames.size())
            {
                pimpl->arrowDown.print();
            }

            pimpl->refreshAlarmFilenameText(filenameIdx);
            pimpl->alarmFilenameText.print();
        }
        else if (pimpl->filenames.empty())
        {
            pimpl->errorText.set("Pas de fichier");
            pimpl->errorText.print();
        }
    }
    else
    {
        pimpl->errorText.set("Pas d'alarme");
        pimpl->errorText.print();
    }

    if (getAlarm(ctx, pimpl->alarmIdx + 1))
    {
        pimpl->arrowRight.print();
    }
    if (getAlarm(ctx, pimpl->alarmIdx - 1))
    {
        pimpl->arrowLeft.print();
    }
}

void ScreenSetAlarmFile::handleClick(Position position)
{
    Audio &audio = ctx.getAudio();
    const bool stopAudio = audio.isPlaying();
    if (stopAudio)
    {
        audio.stopStream();
    }

    switch (position)
    {
    case Position::UpLeft:
        ctx.previousScreen();
        break;
    case Position::UpRight:
        ctx.nextScreen();
        break;
    case Position::Center:
        if (const auto alarm = getAlarm(ctx, pimpl->alarmIdx); alarm != nullptr && stopAudio == false)
        {
            if (const auto configFilename = alarm->getFile(); configFilename.empty() == false)
            {
                const auto filename = ctx.getConfig().getMusic(configFilename);
                audio.loadStream(filename.c_str());
                audio.playStream();
            }
        }
        break;
    case Position::Up:
        advanceAlarm(getAlarm(ctx, pimpl->alarmIdx), pimpl->filenames, 1);
        break;
    case Position::Down:
        advanceAlarm(getAlarm(ctx, pimpl->alarmIdx), pimpl->filenames, -1);
        break;
    case Position::Left:
        if (const size_t newIdx = pimpl->alarmIdx - 1; newIdx < ctx.getConfig().getAlarms().size())
        {
            pimpl->alarmIdx = newIdx;
        }
        break;
    case Position::Right:
        if (const size_t newIdx = pimpl->alarmIdx + 1; newIdx < ctx.getConfig().getAlarms().size())
        {
            pimpl->alarmIdx = newIdx;
        }
        break;
    default:
        break;
    }
}
