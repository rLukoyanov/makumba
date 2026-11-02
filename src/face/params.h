// Параметры лица. Одни и те же структуры описывают и пресеты эмоций (во flash),
// и текущее состояние, которое плавно интерполируется между пресетами.
//
// Соглашения:
//  * координаты в пикселях экрана 128×64, y растёт вниз;
//  * всё, что зависит от стороны (смещение глаза, зрачок, угол века, бровь),
//    задаётся «к носу положительно»: левый и правый глаз зеркальны сами собой;
//  * углы в градусах.
#pragma once

#include <stdint.h>

namespace face
{

    enum class EyeKind : uint8_t
    {
        RRect,   // скруглённый прямоугольник/круг: кольцо (t < min/2) или заливка
        ArcUp,   // ∩ — довольные закрытые глаза «^ ^»
        ArcDown, // ∪ — спящие/умилённые глаза
        Chevron, // > < — зажмурился от смеха или боли
        Heart,
        Star,
        Sparkle, // четырёхлучевая искра
        Spiral,  // головокружение
        Cross,   // x x — «в отключке»
        Dollar,  // $ $
        None,
    };

    struct Eye
    {
        EyeKind kind = EyeKind::RRect;
        float w = 30, h = 30;         // размер
        float r = 15;                 // радиус скругления
        float t = 4;                  // толщина обводки; t >= min(w, h) / 2 — заливка
        float px = 0, py = 0, pr = 0; // зрачок (у кольца) или блик-дырка (у заливки)
        float gx = 0, gy = 0, gr = 0; // второй блик (у заливки)
        float lid_cov = 0;            // верхнее веко: доля закрытия 0..1
        float lid_ang = 0;            // наклон века: + опускает внутренний угол (злость), − (грусть)
        float lid_bend = 0;           // изгиб века: + выгибает аркой вверх
        float low_cov = 0;            // нижнее веко: доля закрытия 0..1
        float low_bend = 0;           // изгиб нижнего века: + полумесяц «улыбающихся глаз»
        float dx = 0, dy = 0;         // смещение от точки раскладки
        float sx = 1, sy = 1;         // масштаб пресета
        float rot = 0;                // поворот значка (звезда, спираль), градусы
        // Только во время работы: моргание и сжатие при саккаде. В пресетах не задаются.
        float squash = 1, stretch = 1;
        bool sq = false; // квадратный зрачок — кибер-глаз редкого брелока

        constexpr Eye pupil(float x, float y, float rad) const
        {
            Eye e = *this;
            e.px = x, e.py = y, e.pr = rad;
            return e;
        }
        constexpr Eye glint(float x, float y, float rad) const { return pupil(x, y, rad); }
        constexpr Eye glint2(float x, float y, float rad) const
        {
            Eye e = *this;
            e.gx = x, e.gy = y, e.gr = rad;
            return e;
        }
        constexpr Eye lid(float cov, float ang = 0, float bend = 0) const
        {
            Eye e = *this;
            e.lid_cov = cov, e.lid_ang = ang, e.lid_bend = bend;
            return e;
        }
        constexpr Eye low(float cov, float bend = 0) const
        {
            Eye e = *this;
            e.low_cov = cov, e.low_bend = bend;
            return e;
        }
        constexpr Eye at(float x, float y) const
        {
            Eye e = *this;
            e.dx = x, e.dy = y;
            return e;
        }
        constexpr Eye scale(float x, float y) const
        {
            Eye e = *this;
            e.sx = x, e.sy = y;
            return e;
        }
        constexpr Eye radius(float rad) const
        {
            Eye e = *this;
            e.r = rad;
            return e;
        }
        constexpr Eye rotate(float deg) const
        {
            Eye e = *this;
            e.rot = deg;
            return e;
        }
    };

    struct Brow
    {
        float len = 0;  // длина; 0 — бровей нет
        float t = 3;    // толщина
        float dx = 0;   // сдвиг к носу
        float gap = 4;  // зазор над верхом глаза
        float ang = 0;  // + поднят внутренний конец (грусть, тревога), − опущен (злость)
        float bend = 0; // + выгнута аркой

        constexpr Brow at(float x, float g) const
        {
            Brow b = *this;
            b.dx = x, b.gap = g;
            return b;
        }
    };

    enum class MouthKind : uint8_t
    {
        None,
        Curve,    // линия, улыбка, грусть; при open > 0 — открытый рот («D»)
        Ring,     // «o»/«O»
        Wavy,     // волнистый — нервы, смятение
        Cat,      // «ω»
        Pucker,   // «3» — поцелуй
        Teeth,    // оскал/гримаса
        Tongue,   // показывает язык
        Triangle, // крик
        Scream,   // открытый рот-контур с верхними зубами (сойджак)
        Rage,     // открытый рот-контур с зубами сверху и снизу (FFFUUU)
        Grin,     // широкая зубастая ухмылка
        Box,      // «□»
    };

    struct Mouth
    {
        MouthKind kind = MouthKind::Curve;
        float w = 18;    // ширина
        float curve = 0; // прогиб центра: + улыбка, − грусть
        float open = 0;  // раскрытие (для Curve)
        float t = 3;     // толщина линии
        float tilt = 0;  // перекос: + правый угол ниже
        float h = 0;     // высота Ring/Cat/Pucker/Teeth/Triangle/Box, амплитуда Wavy, зубы Scream/Rage
        float dx = 0, dy = 0;
        float sx = 1; // масштаб (для смены формы рта)

        constexpr Mouth at(float x, float y) const
        {
            Mouth m = *this;
            m.dx = x, m.dy = y;
            return m;
        }
        constexpr Mouth skew(float v) const
        {
            Mouth m = *this;
            m.tilt = v;
            return m;
        }
        constexpr Mouth thick(float v) const
        {
            Mouth m = *this;
            m.t = v;
            return m;
        }
    };

    struct Layout
    {
        float spacing = 54; // между центрами глаз
        float eye_y = 27;
        float mouth_y = 49;
        float tilt = 0; // наклон головы: левый глаз выше на tilt, правый ниже
    };

    struct EyePair
    {
        Eye l, r;
    };
    struct BrowPair
    {
        Brow l, r;
    };

    // Полное состояние лица: пресет плюс процедурные слои.
    struct Face
    {
        Eye eye[2];
        Brow brow[2];
        Mouth mouth;
        Layout layout;
        float ox = 0, oy = 0; // смещение всего лица (дрейф, движение, клипы)
        float gx = 0, gy = 0; // взгляд, −1..1
    };

    // ---- Конструкторы для пресетов ----

    constexpr Eye ring(float d, float t = 4) { return Eye{EyeKind::RRect, d, d, d * 0.5f, t}; }
    constexpr Eye ring_wh(float w, float h, float t = 4) { return Eye{EyeKind::RRect, w, h, (w < h ? w : h) * 0.5f, t}; }
    constexpr Eye filled(float w, float h, float r = -1)
    {
        return Eye{EyeKind::RRect, w, h, r < 0 ? (w < h ? w : h) * 0.5f : r, 99};
    }
    constexpr Eye arc_up(float w, float h, float t = 4) { return Eye{EyeKind::ArcUp, w, h, 0, t}; }
    constexpr Eye arc_down(float w, float h, float t = 4) { return Eye{EyeKind::ArcDown, w, h, 0, t}; }
    constexpr Eye chevron(float w, float h, float t = 4) { return Eye{EyeKind::Chevron, w, h, 0, t}; }
    constexpr Eye heart(float w, float h) { return Eye{EyeKind::Heart, w, h, 0, 99}; }
    constexpr Eye heart_ring(float w, float h, float t = 3) { return Eye{EyeKind::Heart, w, h, 0, t}; }
    constexpr Eye star(float d) { return Eye{EyeKind::Star, d, d, 0, 99}; }
    constexpr Eye sparkle(float d) { return Eye{EyeKind::Sparkle, d, d, 0, 99}; }
    constexpr Eye spiral(float d, float t = 2) { return Eye{EyeKind::Spiral, d, d, 0, t}; }
    constexpr Eye cross(float w, float h, float t = 4) { return Eye{EyeKind::Cross, w, h, 0, t}; }
    constexpr Eye dollar(float w, float h, float t = 3) { return Eye{EyeKind::Dollar, w, h, 0, t}; }
    constexpr Eye no_eye() { return Eye{EyeKind::None, 0, 0, 0, 0}; }

    constexpr EyePair both(Eye e) { return EyePair{e, e}; }
    constexpr EyePair pair(Eye l, Eye r) { return EyePair{l, r}; }

    constexpr Brow brow(float len, float ang = 0, float bend = 0, float t = 3) { return Brow{len, t, 0, 4, ang, bend}; }
    constexpr BrowPair brows(Brow b) { return BrowPair{b, b}; }
    constexpr BrowPair brows(Brow l, Brow r) { return BrowPair{l, r}; }

    constexpr Mouth no_mouth() { return Mouth{MouthKind::None, 0}; }
    constexpr Mouth line(float w, float t = 3) { return Mouth{MouthKind::Curve, w, 0, 0, t}; }
    constexpr Mouth smile(float w, float curve, float t = 3) { return Mouth{MouthKind::Curve, w, curve, 0, t}; }
    constexpr Mouth open(float w, float curve, float op, float t = 3) { return Mouth{MouthKind::Curve, w, curve, op, t}; }
    constexpr Mouth o_mouth(float w, float h, float t = 3) { return Mouth{MouthKind::Ring, w, 0, 0, t, 0, h}; }
    constexpr Mouth wavy(float w, float amp, float t = 2) { return Mouth{MouthKind::Wavy, w, 0, 0, t, 0, amp}; }
    constexpr Mouth cat(float w, float h, float t = 2) { return Mouth{MouthKind::Cat, w, 0, 0, t, 0, h}; }
    constexpr Mouth pucker(float h, float t = 2) { return Mouth{MouthKind::Pucker, h * 0.6f, 0, 0, t, 0, h}; }
    constexpr Mouth teeth(float w, float h) { return Mouth{MouthKind::Teeth, w, 0, 0, 2, 0, h}; }
    constexpr Mouth tongue(float w, float curve, float h = 6) { return Mouth{MouthKind::Tongue, w, curve, 0, 3, 0, h}; }
    constexpr Mouth shout(float w, float h) { return Mouth{MouthKind::Triangle, w, 0, 0, 3, 0, h}; }
    // Открытый рот-контур: curve — прогиб верхней кромки, op — глубина, teeth — высота зубов.
    constexpr Mouth scream(float w, float curve, float op, float teeth = 3, float t = 2)
    {
        return Mouth{MouthKind::Scream, w, curve, op, t, 0, teeth};
    }
    constexpr Mouth rage_mouth(float w, float curve, float op, float teeth = 3, float t = 2)
    {
        return Mouth{MouthKind::Rage, w, curve, op, t, 0, teeth};
    }
    constexpr Mouth grin(float w, float curve, float op, float t = 2) { return Mouth{MouthKind::Grin, w, curve, op, t}; }
    constexpr Mouth box(float w, float h, float t = 2) { return Mouth{MouthKind::Box, w, 0, 0, t, 0, h}; }

    constexpr Layout layout(float spacing = 54, float eye_y = 27, float mouth_y = 49, float tilt = 0)
    {
        return Layout{spacing, eye_y, mouth_y, tilt};
    }

} // namespace face