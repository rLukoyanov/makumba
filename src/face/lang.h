// Язык текстов движка для симулятора, документации и Serial: русский или английский.
// На экран брелока это не влияет: надписи на OLED и так латиницей.
#pragma once

#include <stdint.h>

namespace face
{

    enum class Lang : uint8_t
    {
        Ru,
        En,
        Count
    };

    // Общий выбор языка. По умолчанию русский — так, как было до перевода.
    void set_lang(Lang l);
    Lang lang();
    const char *lang_id(Lang l); // "ru", "en"
    bool lang_from_id(const char *id, Lang *out);
    // Выбирает строку по языку; пустой перевод заменяется русской строкой.
    inline const char *pick_lang(Lang l, const char *ru, const char *en)
    {
        return l == Lang::En && en && *en ? en : ru;
    }

    // Причина последней реакции (Behavior::last_reason) на заданном языке. Русская строка —
    // ключ; если перевода нет, возвращается она же.
    const char *reason_text(const char *ru, Lang l);
    inline const char *reason_text(const char *ru) { return reason_text(ru, lang()); }
    bool reason_translated(const char *ru); // есть ли английский перевод (для тестов)

} // namespace face