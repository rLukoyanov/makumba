// Каталог эмоций: идентификаторы, оверлеи, движения и сами пресеты.
#pragma once

#include <stdint.h>

#include "face/lang.h"
#include "face/params.h"

namespace face
{

    // Символические значки поверх лица (манга-«манпу»).
    enum class Overlay : uint8_t
    {
        BlushLines,  // румянец «///»
        BlushDither, // румянец-овал через дизеринг
        Sweat,       // капля пота
        Vein,        // «💢» — вздувшаяся вена
        Tear,        // слезинки
        TearStream,  // ручьи слёз
        Zzz,
        Hearts,      // всплывающие сердечки
        Sparkles,    // искры
        SoundWaves,  // звуковые волны у глаз
        Notes,       // ноты
        Question,    // «?»
        Exclaim,     // «!»
        Ellipsis,    // «...»
        Steam,       // пар от злости
        Gloom,       // линии уныния
        DizzyStars,  // звёздочки по орбите
        Bulb,        // лампочка-идея
        Shades,      // тёмные очки
        Bolt,        // молния зарядки
        Snow,        // снежинки
        Drool,       // слюнка
        Rays,        // линии испуга
        BatteryLow,  // значок батареи
        SpeedLines,  // линии скорости
        Bubble,      // пузырь из носа во сне
        Halo,        // нимб
        Horns,       // рожки
        Glasses,     // очки в тонкой оправе
        Nose,        // нос (для мемных лиц)
        Stubble,     // щетина
        Stick,       // ватная палочка во рту
        EyeBags,     // мешки под глазами
        ShrugArms,   // ¯\_ _/¯
        Explosion,   // «мозг взорван»
        Bat,         // летучая мышь (хэллоуин)
        GameOver,    // надпись GAME OVER
        QueasyLines, // волнистые линии недомогания
        Count
    };

    constexpr uint64_t ov() { return 0; }
    template <typename... Rest>
    constexpr uint64_t ov(Overlay first, Rest... rest)
    {
        return (1ull << (uint64_t)first) | ov(rest...);
    }
    static_assert((int)Overlay::Count <= 64, "маска оверлеев — 64 бита");

    // Непрерывное движение всего лица, пока держится эмоция.
    enum class Motion : uint8_t
    {
        None,
        Bounce,  // подпрыгивает
        Bob,     // плавно покачивается вверх-вниз
        Tremble, // дрожит
        Shake,   // трясёт головой
        Sway,    // покачивается из стороны в сторону, с наклоном
        Shiver,  // мелко дрожит от холода
        Pulse,   // глаза пульсируют, как сердце
        Float,   // мечтательно плывёт
        Nod,     // кивает в ритм
        Wobble,  // качается, как неваляшка
    };

    // Как ведёт себя взгляд.
    enum class Gaze : uint8_t
    {
        Idle,   // обычные саккады: чаще в центр, иногда по сторонам
        Still,  // почти неподвижен
        Around, // осматривается
        Up,
        Down,
        Left,
        Right,
        Avert,  // отводит взгляд вниз-вбок
        Dart,   // нервно бегает
        UpSide, // вверх и вбок — задумался
    };

    enum class Blink : uint8_t
    {
        None,
        Rare,
        Normal,
        Often,
        Slow
    };

    // Фирменное движение эмоции: короткий жест раз в accent_ms (±35 %).
    // Благодаря ему ни одна эмоция не стоит мёртво, а кадры нужны только во время жеста.
    enum class Accent : uint8_t
    {
        Breath, // лёгкое «дыхание» глаз и рта
        None,
        Brows,   // вскидывает брови
        Chatter, // шевелит ртом, как будто говорит
        Squint,  // прищуривается
        Hop,     // подпрыгивает
        Tilt,    // наклоняет голову
        Look,    // косится в сторону и обратно
        Wiggle,  // глаза мелко качаются
        Pulse,   // глаза пульсируют
        Twitch,  // дёргается глаз
        Nod,     // кивает
        Shake,   // мотает головой
        Glint,   // зрачки блестят
        Grin,    // улыбка шире
        Sway,    // качается из стороны в сторону
        Bob,     // плавно приподнимается
    };

    enum class Category : uint8_t
    {
        Positive,
        Negative,
        Surprise,
        Cognitive,
        Social,
        State,
        System,
        Fun,
        Meme,
        Sensor,
        Count
    };

// Список эмоций. Порядок совпадает с таблицей пресетов в expressions.cpp.
#define FACE_EXPR_LIST(X) \
    X(neutral)            \
    X(calm)               \
    X(happy)              \
    X(joy)                \
    X(laugh)              \
    X(giggle)             \
    X(love)               \
    X(adoring)            \
    X(excited)            \
    X(starstruck)         \
    X(proud)              \
    X(wink)               \
    X(playful)            \
    X(kiss)               \
    X(grateful)           \
    X(relieved)           \
    X(content)            \
    X(sad)                \
    X(crying)             \
    X(sobbing)            \
    X(disappointed)       \
    X(lonely)             \
    X(hurt)               \
    X(angry)              \
    X(furious)            \
    X(annoyed)            \
    X(grumpy)             \
    X(disgusted)          \
    X(sulking)            \
    X(embarrassed)        \
    X(guilty)             \
    X(surprised)          \
    X(shocked)            \
    X(amazed)             \
    X(scared)             \
    X(terrified)          \
    X(nervous)            \
    X(worried)            \
    X(startled)           \
    X(curious)            \
    X(confused)           \
    X(thinking)           \
    X(idea)               \
    X(focused)            \
    X(skeptical)          \
    X(suspicious)         \
    X(listening)          \
    X(mischievous)        \
    X(determined)         \
    X(daydreaming)        \
    X(processing)         \
    X(hello)              \
    X(goodbye)            \
    X(pleading)           \
    X(shy)                \
    X(smug)               \
    X(innocent)           \
    X(devilish)           \
    X(apologetic)         \
    X(sleepy)             \
    X(sleeping)           \
    X(waking)             \
    X(bored)              \
    X(tired)              \
    X(exhausted)          \
    X(dizzy)              \
    X(knocked_out)        \
    X(hungry)             \
    X(cold)               \
    X(hot)                \
    X(sick)               \
    X(dazzled)            \
    X(afraid_dark)        \
    X(charging)           \
    X(charged)            \
    X(low_battery)        \
    X(critical_battery)   \
    X(error)              \
    X(music)              \
    X(singing)            \
    X(cool)               \
    X(uwu)                \
    X(owo)                \
    X(derp)               \
    X(robot)              \
    X(party)              \
    X(swab_cat)           \
    X(soy_scream)         \
    X(soy_wow)            \
    X(smug_glasses)       \
    X(hmm_squint)         \
    X(rage)               \
    X(me_gusta)           \
    X(lenny)              \
    X(disapproval)        \
    X(unamused)           \
    X(awkward)            \
    X(wtf)                \
    X(table_flip)         \
    X(shrug)              \
    X(troll_grin)         \
    X(poker_face)         \
    X(not_bad)            \
    X(challenge_accepted) \
    X(tears_of_joy)       \
    X(im_dead)            \
    X(pog)                \
    X(chad_stare)         \
    X(npc)                \
    X(doomer)             \
    X(bruh)               \
    X(cringe)             \
    X(money)              \
    X(vibing)             \
    X(hypnotized)         \
    X(screaming)          \
    X(nervous_laugh)      \
    X(evil_laugh)         \
    X(zoned_out)          \
    X(flustered)          \
    X(speechless)         \
    X(mind_blown)         \
    X(boo)                \
    X(twitchy)            \
    X(sleep_deprived)     \
    X(bliss)              \
    X(touched)            \
    X(alert)              \
    X(calling)            \
    X(walking)            \
    X(running)            \
    X(queasy)             \
    X(upside_down)        \
    X(pocket_peek)        \
    X(squint)             \
    X(booped)             \
    X(game_over)          \
    X(cyclops)            \
    X(spooky)

    enum class Expr : uint8_t
    {
#define FACE_X(n) n,
        FACE_EXPR_LIST(FACE_X)
#undef FACE_X
        Count
    };

    constexpr int kExprCount = (int)Expr::Count;

    struct Expression
    {
        const char *id;
        const char *name;         // по-русски, для симулятора и документации
        const char *name_en = ""; // то же по-английски (expr_name выбирает по lang())
        Category cat = Category::Positive;
        EyePair eyes = both(ring(30, 4));
        BrowPair brows = {};
        Mouth mouth = line(18);
        Layout lay = {};
        uint64_t overlays = 0;
        Motion motion = Motion::None;
        float motion_amp = 0; // пиксели (для Pulse — проценты)
        float motion_hz = 0;
        Gaze gaze = Gaze::Idle;
        Blink blink = Blink::Normal;
        Accent accent = Accent::Breath;
        uint16_t accent_ms = 4000; // как часто повторяется фирменное движение
        uint16_t in_ms = 260;      // длительность перехода в эмоцию
        uint16_t hold_ms = 0;      // сколько держится (0 — пока не сменят)
        Expr next = Expr::neutral; // во что переходит после hold_ms
        const char *when = "";     // в каких ситуациях
        const char *when_en = "";  // то же по-английски
    };

    const Expression &expression(Expr e);
    const char *expr_id(Expr e);
    // Название и описание на языке lang() (по умолчанию — по-русски) или на заданном.
    const char *expr_name(Expr e);
    const char *expr_name(Expr e, Lang l);
    const char *expr_when(Expr e);
    const char *expr_when(Expr e, Lang l);
    const char *category_name(Category c); // на языке lang()
    const char *category_name(Category c, Lang l);
    bool expr_from_id(const char *id, Expr *out);
    // Базовая форма лица для эмоции (без процедурных слоёв).
    void expression_face(Expr e, Face *out);

} // namespace face