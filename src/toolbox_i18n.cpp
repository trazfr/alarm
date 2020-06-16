#include "toolbox_i18n.hpp"

#include <libintl.h>

#include <clocale>
#include <ostream>

namespace
{
static constexpr char kDomain[] = "alarm";
} // namespace

Internationalization::Internationalization()
{
    std::setlocale(LC_ALL, "");
}

Internationalization::~Internationalization() = default;

void Internationalization::setMessagesFolder(const char *folder)
{
    bindtextdomain(kDomain, folder);
    textdomain(kDomain);
}

const char *Internationalization::getLocale() const
{
    return std::setlocale(LC_ALL, nullptr);
}

const char *Internationalization::translate(const char *msgid)
{
    return gettext(msgid);
}

std::ostream &Internationalization::toStream(std::ostream &str) const
{
    return str << "locale=" << getLocale() << " domain=" << kDomain << " folder=" << bindtextdomain(kDomain, nullptr);
}
