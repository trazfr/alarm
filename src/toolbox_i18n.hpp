#include <memory>

#define _(x) Internationalization::translate(x)

class Internationalization
{
public:
    Internationalization();
    ~Internationalization();

    void setMessagesFolder(const char *folder);

    /**
     * Get the current locale in use
     */
    const char *getLocale() const;

    static const char *translate(const char *msgid);

    friend std::ostream &operator<<(std::ostream &str, const Internationalization &i18n)
    {
        return i18n.toStream(str);
    }

private:
    std::ostream &toStream(std::ostream &str) const;
};
