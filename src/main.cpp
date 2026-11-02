#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "face/expressions.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
bool useSprite = false;

const int PHOTORESISTOR_PIN = 27;
constexpr bool kDarkWhenLow = false;
constexpr uint32_t kLightDebounceMs = 900;

constexpr int kScreenW = 240;
constexpr int kScreenH = 240;
constexpr int kCenterX = kScreenW / 2;
constexpr int kCenterY = 100;
constexpr int kEyeOffsetX = 55;
constexpr int kMouthBaseY = 175;
constexpr bool kInvertDisplay = true;

constexpr int kTargetFps = 120;
const int fpsInterval = 1000 / kTargetFps;
unsigned long nextEmotionAt = 0;
unsigned long exprStartedAt = 0;
float mouthWave = 0;
face::Expr currentExpr = face::Expr::neutral;

constexpr int kRecentN = 8;
face::Expr recentExpr[kRecentN] = {};
int recentCount = 0;
int recentPos = 0;

void pushRecent(face::Expr e);

bool lightRawDark = false;
bool lightStableDark = false;
unsigned long lightLastEdgeAt = 0;

void setExpression(face::Expr e, uint32_t now)
{
  currentExpr = e;
  pushRecent(currentExpr);
  exprStartedAt = now;
  const face::Expression &nx = face::expression(currentExpr);
  nextEmotionAt = now + (nx.hold_ms ? nx.hold_ms : (uint16_t)(1800 + random(2200)));
}

bool updateLightState(uint32_t now)
{
  const int level = digitalRead(PHOTORESISTOR_PIN);
  const bool rawDark = kDarkWhenLow ? (level == LOW) : (level == HIGH);

  if (rawDark != lightRawDark)
  {
    lightRawDark = rawDark;
    lightLastEdgeAt = now;
  }

  if (rawDark != lightStableDark && (now - lightLastEdgeAt) >= kLightDebounceMs)
  {
    lightStableDark = rawDark;
    return true;
  }

  return false;
}

struct CatWeight
{
  face::Category cat;
  float w;
};

bool isRecent(face::Expr e)
{
  for (int i = 0; i < recentCount; ++i)
  {
    if (recentExpr[i] == e)
      return true;
  }
  return false;
}

void pushRecent(face::Expr e)
{
  recentExpr[recentPos] = e;
  recentPos = (recentPos + 1) % kRecentN;
  if (recentCount < kRecentN)
    ++recentCount;
}

face::Expr pickFromCategory(face::Category cat, bool avoidRecent)
{
  int idx[face::kExprCount];
  int n = 0;
  for (int i = 0; i < face::kExprCount; ++i)
  {
    const face::Expr e = static_cast<face::Expr>(i);
    const face::Expression &x = face::expression(e);
    if (x.cat != cat)
      continue;
    if (avoidRecent && isRecent(e))
      continue;
    idx[n++] = i;
  }
  if (n == 0)
    return face::Expr::neutral;
  return static_cast<face::Expr>(idx[random(n)]);
}

face::Expr pickAny(bool avoidRecent)
{
  int idx[face::kExprCount];
  int n = 0;
  for (int i = 0; i < face::kExprCount; ++i)
  {
    const face::Expr e = static_cast<face::Expr>(i);
    if (avoidRecent && isRecent(e))
      continue;
    idx[n++] = i;
  }
  if (n == 0)
    return face::Expr::neutral;
  return static_cast<face::Expr>(idx[random(n)]);
}

face::Expr chooseBehaviorExpr(uint32_t now)
{
  const float t = now * 0.001f;
  const float calm = (sin(t * 0.05f) + 1.0f) * 0.5f;
  const float fun = (sin(t * 0.09f + 1.7f) + 1.0f) * 0.5f;

  CatWeight pool[] = {
      {face::Category::Positive, 2.6f + calm * 1.8f},
      {face::Category::Cognitive, 2.0f},
      {face::Category::Social, 1.4f},
      {face::Category::State, 1.2f + (1.0f - calm)},
      {face::Category::Fun, 1.2f + fun * 1.8f},
      {face::Category::Surprise, 0.9f},
      {face::Category::Meme, 0.9f + fun * 1.1f},
      {face::Category::Negative, 0.8f + (1.0f - calm) * 1.0f},
      {face::Category::Sensor, 0.8f},
      {face::Category::System, 0.5f}};

  float total = 0.0f;
  for (const CatWeight &cw : pool)
    total += cw.w;

  float r = ((float)random(10000) / 10000.0f) * total;
  face::Category chosen = face::Category::Positive;
  for (const CatWeight &cw : pool)
  {
    r -= cw.w;
    if (r <= 0.0f)
    {
      chosen = cw.cat;
      break;
    }
  }

  face::Expr e = pickFromCategory(chosen, true);
  if (e == face::Expr::neutral && chosen != face::Category::Positive)
    e = pickFromCategory(chosen, false);
  if (e == face::Expr::neutral)
    e = pickAny(true);
  if (e == face::Expr::neutral)
    e = pickAny(false);
  return e;
}

void advanceBehavior(uint32_t now)
{
  if (now < nextEmotionAt)
    return;

  const face::Expression &cur = face::expression(currentExpr);

  if (cur.hold_ms > 0 && now - exprStartedAt >= cur.hold_ms)
  {
    if (cur.next != currentExpr)
    {
      currentExpr = cur.next;
      pushRecent(currentExpr);
      exprStartedAt = now;
      const face::Expression &nx = face::expression(currentExpr);
      nextEmotionAt = now + (nx.hold_ms ? nx.hold_ms : (uint16_t)(1800 + random(2200)));
      return;
    }
  }

  face::Expr next = chooseBehaviorExpr(now);
  if (next == currentExpr)
    next = pickAny(true);
  if (next == currentExpr)
    next = pickAny(false);

  setExpression(next, now);
}

float triangleWave(float x)
{
  const float f = x - floorf(x);
  return 1.0f - fabsf(f * 2.0f - 1.0f);
}

float blinkStrength(const face::Expression &expr, float t)
{
  float period = 0.0f;
  switch (expr.blink)
  {
  case face::Blink::None:
    return 0.0f;
  case face::Blink::Rare:
    period = 7.0f;
    break;
  case face::Blink::Normal:
    period = 4.2f;
    break;
  case face::Blink::Often:
    period = 2.4f;
    break;
  case face::Blink::Slow:
    period = 5.2f;
    break;
  }

  const float phase = fmodf(t, period) / period;
  if (phase < 0.86f)
    return 0.0f;
  return triangleWave((phase - 0.86f) / 0.14f);
}

float accentStrength(const face::Expression &expr, float t)
{
  if (expr.accent == face::Accent::None || expr.accent_ms == 0)
    return 0.0f;
  const float period = expr.accent_ms * 0.001f;
  const float phase = fmodf(t, period) / period;
  if (phase < 0.62f)
    return 0.0f;
  return triangleWave((phase - 0.62f) / 0.38f);
}

bool hasOverlay(const face::Expression &expr, face::Overlay ov)
{
  return (expr.overlays & (1ull << (uint64_t)ov)) != 0;
}

template <typename Canvas>
void drawArcStroke(Canvas &canvas, int cx, int cy, int rx, int ry, float a0deg, float a1deg, uint16_t color, int thick)
{
  const int steps = 20;
  const float a0 = a0deg * PI / 180.0f;
  const float a1 = a1deg * PI / 180.0f;
  int px = cx + (int)(cosf(a0) * rx);
  int py = cy + (int)(sinf(a0) * ry);
  for (int i = 1; i <= steps; ++i)
  {
    const float a = a0 + (a1 - a0) * ((float)i / (float)steps);
    const int x = cx + (int)(cosf(a) * rx);
    const int y = cy + (int)(sinf(a) * ry);
    if (thick <= 1)
    {
      canvas.drawLine(px, py, x, y, color);
    }
    else
    {
      canvas.fillCircle(px, py, thick / 2, color);
      canvas.fillCircle(x, y, thick / 2, color);
      canvas.drawLine(px, py, x, y, color);
    }
    px = x;
    py = y;
  }
}

template <typename Canvas>
void drawOverlays(Canvas &canvas, const face::Expression &expr, int centerX, int eyeY, int mouthY, uint16_t color, float t)
{
  if (expr.overlays == 0)
    return;

  if (hasOverlay(expr, face::Overlay::BlushLines))
  {
    for (int i = 0; i < 3; ++i)
    {
      canvas.drawLine(centerX - 90 + i * 6, mouthY - 6, centerX - 82 + i * 6, mouthY + 4, color);
      canvas.drawLine(centerX + 90 - i * 6, mouthY - 6, centerX + 82 - i * 6, mouthY + 4, color);
    }
  }
  if (hasOverlay(expr, face::Overlay::BlushDither))
  {
    for (int y = -8; y <= 8; ++y)
    {
      for (int x = -18; x <= 18; ++x)
      {
        if (((x + y) & 1) == 0 && x * x + y * y < 260)
        {
          canvas.drawPixel(centerX - 85 + x, mouthY + y, color);
          canvas.drawPixel(centerX + 85 + x, mouthY + y, color);
        }
      }
    }
  }
  if (hasOverlay(expr, face::Overlay::Sweat))
  {
    canvas.drawCircle(centerX + 90, eyeY - 8, 7, color);
    canvas.drawLine(centerX + 90, eyeY - 15, centerX + 90, eyeY - 3, color);
  }
  if (hasOverlay(expr, face::Overlay::Vein))
  {
    const int vx = centerX + 86;
    const int vy = eyeY - 40;
    canvas.drawLine(vx - 8, vy, vx + 8, vy, color);
    canvas.drawLine(vx, vy - 8, vx, vy + 8, color);
    canvas.drawLine(vx - 6, vy - 6, vx + 6, vy + 6, color);
    canvas.drawLine(vx + 6, vy - 6, vx - 6, vy + 6, color);
  }
  if (hasOverlay(expr, face::Overlay::Tear))
  {
    canvas.fillCircle(centerX + 66, eyeY + 26, 4, color);
    canvas.drawLine(centerX + 66, eyeY + 20, centerX + 66, eyeY + 30, color);
  }
  if (hasOverlay(expr, face::Overlay::TearStream))
  {
    canvas.drawLine(centerX - 54, eyeY + 12, centerX - 54, eyeY + 45, color);
    canvas.drawLine(centerX + 54, eyeY + 12, centerX + 54, eyeY + 45, color);
  }
  if (hasOverlay(expr, face::Overlay::Zzz))
  {
    canvas.drawString("Z", centerX + 72, eyeY - 56, 2);
    canvas.drawString("z", centerX + 92, eyeY - 72, 2);
    canvas.drawString("z", centerX + 108, eyeY - 86, 2);
  }
  if (hasOverlay(expr, face::Overlay::Hearts))
  {
    canvas.drawString("<3", centerX - 98, eyeY - 58, 2);
    canvas.drawString("<3", centerX + 68, eyeY - 72, 2);
  }
  if (hasOverlay(expr, face::Overlay::Sparkles))
  {
    const int s = (int)(2 + fabsf(sinf(t * 6.0f)) * 3.0f);
    canvas.drawLine(centerX - 92 - s, eyeY - 48, centerX - 92 + s, eyeY - 48, color);
    canvas.drawLine(centerX - 92, eyeY - 48 - s, centerX - 92, eyeY - 48 + s, color);
    canvas.drawLine(centerX + 90 - s, eyeY - 58, centerX + 90 + s, eyeY - 58, color);
    canvas.drawLine(centerX + 90, eyeY - 58 - s, centerX + 90, eyeY - 58 + s, color);
  }
  if (hasOverlay(expr, face::Overlay::SoundWaves))
  {
    drawArcStroke(canvas, centerX - 102, eyeY, 10, 16, -60, 60, color, 2);
    drawArcStroke(canvas, centerX + 102, eyeY, 10, 16, 120, 240, color, 2);
  }
  if (hasOverlay(expr, face::Overlay::Notes))
  {
    canvas.drawCircle(centerX + 86, eyeY - 58, 4, color);
    canvas.drawLine(centerX + 90, eyeY - 58, centerX + 90, eyeY - 78, color);
    canvas.drawLine(centerX + 90, eyeY - 78, centerX + 98, eyeY - 75, color);
  }
  if (hasOverlay(expr, face::Overlay::Question))
    canvas.drawString("?", centerX + 90, eyeY - 64, 2);
  if (hasOverlay(expr, face::Overlay::Exclaim))
    canvas.drawString("!", centerX + 90, eyeY - 64, 2);
  if (hasOverlay(expr, face::Overlay::Ellipsis))
    canvas.drawString("...", centerX + 72, mouthY + 18, 2);
  if (hasOverlay(expr, face::Overlay::Steam))
  {
    drawArcStroke(canvas, centerX + 96, eyeY - 22, 8, 16, 200, 340, color, 2);
    drawArcStroke(canvas, centerX + 108, eyeY - 30, 8, 16, 200, 340, color, 2);
  }
  if (hasOverlay(expr, face::Overlay::Gloom))
  {
    for (int i = 0; i < 4; ++i)
      canvas.drawLine(centerX - 95 + i * 14, eyeY - 56, centerX - 100 + i * 14, eyeY - 30, color);
  }
  if (hasOverlay(expr, face::Overlay::DizzyStars))
  {
    canvas.drawString("*", centerX - 100, eyeY - 40, 2);
    canvas.drawString("*", centerX + 92, eyeY - 28, 2);
  }
  if (hasOverlay(expr, face::Overlay::Bulb))
  {
    canvas.drawCircle(centerX, eyeY - 60, 10, color);
    canvas.drawRect(centerX - 4, eyeY - 48, 8, 6, color);
  }
  if (hasOverlay(expr, face::Overlay::Shades))
  {
    canvas.drawRect(centerX - 74, eyeY - 18, 42, 24, color);
    canvas.drawRect(centerX + 32, eyeY - 18, 42, 24, color);
    canvas.drawLine(centerX - 32, eyeY - 6, centerX + 32, eyeY - 6, color);
  }
  if (hasOverlay(expr, face::Overlay::Bolt))
  {
    int bx = centerX + 88;
    int by = mouthY - 8;
    canvas.drawLine(bx, by - 10, bx - 6, by + 2, color);
    canvas.drawLine(bx - 6, by + 2, bx + 2, by + 2, color);
    canvas.drawLine(bx + 2, by + 2, bx - 4, by + 14, color);
  }
  if (hasOverlay(expr, face::Overlay::Snow))
  {
    canvas.drawString("*", centerX - 96, eyeY - 52, 2);
    canvas.drawString("*", centerX + 88, eyeY - 62, 2);
    canvas.drawString("*", centerX + 100, eyeY - 40, 2);
  }
  if (hasOverlay(expr, face::Overlay::Drool))
  {
    canvas.drawLine(centerX + 22, mouthY + 6, centerX + 24, mouthY + 24, color);
    canvas.fillCircle(centerX + 24, mouthY + 24, 3, color);
  }
  if (hasOverlay(expr, face::Overlay::Rays))
  {
    for (int i = 0; i < 5; ++i)
    {
      const float a = (-40.0f + i * 20.0f) * PI / 180.0f;
      canvas.drawLine(centerX + (int)(cosf(a) * 82), eyeY - 6 + (int)(sinf(a) * 52), centerX + (int)(cosf(a) * 110), eyeY - 6 + (int)(sinf(a) * 72), color);
    }
  }
  if (hasOverlay(expr, face::Overlay::BatteryLow))
  {
    canvas.drawRect(centerX + 72, mouthY - 14, 28, 14, color);
    canvas.drawRect(centerX + 100, mouthY - 10, 3, 6, color);
    canvas.fillRect(centerX + 75, mouthY - 11, 6, 8, color);
  }
  if (hasOverlay(expr, face::Overlay::SpeedLines))
  {
    canvas.drawLine(centerX - 112, eyeY - 10, centerX - 84, eyeY - 10, color);
    canvas.drawLine(centerX - 110, eyeY + 8, centerX - 86, eyeY + 8, color);
    canvas.drawLine(centerX - 108, eyeY + 24, centerX - 88, eyeY + 24, color);
  }
  if (hasOverlay(expr, face::Overlay::Bubble))
    canvas.drawCircle(centerX + 32, mouthY + 10, 6, color);
  if (hasOverlay(expr, face::Overlay::Halo))
    canvas.drawEllipse(centerX, eyeY - 56, 38, 10, color);
  if (hasOverlay(expr, face::Overlay::Horns))
  {
    canvas.drawTriangle(centerX - 56, eyeY - 46, centerX - 36, eyeY - 60, centerX - 26, eyeY - 40, color);
    canvas.drawTriangle(centerX + 56, eyeY - 46, centerX + 36, eyeY - 60, centerX + 26, eyeY - 40, color);
  }
  if (hasOverlay(expr, face::Overlay::Glasses))
  {
    canvas.drawRect(centerX - 70, eyeY - 12, 38, 22, color);
    canvas.drawRect(centerX + 32, eyeY - 12, 38, 22, color);
    canvas.drawLine(centerX - 32, eyeY - 1, centerX + 32, eyeY - 1, color);
  }
  if (hasOverlay(expr, face::Overlay::Nose))
  {
    canvas.drawLine(centerX, eyeY + 4, centerX - 2, eyeY + 20, color);
    canvas.drawLine(centerX - 2, eyeY + 20, centerX + 6, eyeY + 22, color);
  }
  if (hasOverlay(expr, face::Overlay::Stubble))
  {
    for (int i = -2; i <= 2; ++i)
      canvas.drawPixel(centerX + i * 5, mouthY + 16, color);
  }
  if (hasOverlay(expr, face::Overlay::Stick))
    canvas.drawLine(centerX + 6, mouthY + 2, centerX + 42, mouthY + 8, color);
  if (hasOverlay(expr, face::Overlay::EyeBags))
  {
    drawArcStroke(canvas, centerX - 55, eyeY + 20, 16, 6, 20, 160, color, 2);
    drawArcStroke(canvas, centerX + 55, eyeY + 20, 16, 6, 20, 160, color, 2);
  }
  if (hasOverlay(expr, face::Overlay::ShrugArms))
  {
    canvas.drawLine(centerX - 56, mouthY + 14, centerX - 24, mouthY + 6, color);
    canvas.drawLine(centerX + 56, mouthY + 14, centerX + 24, mouthY + 6, color);
  }
  if (hasOverlay(expr, face::Overlay::Explosion))
  {
    for (int i = 0; i < 8; ++i)
    {
      const float a = i * (2.0f * PI / 8.0f);
      canvas.drawLine(centerX, eyeY - 60, centerX + (int)(cosf(a) * 22), eyeY - 60 + (int)(sinf(a) * 22), color);
    }
  }
  if (hasOverlay(expr, face::Overlay::Bat))
  {
    canvas.drawArc(centerX, eyeY - 62, 18, 12, 200, 340, color, TFT_BLACK);
    canvas.drawArc(centerX - 14, eyeY - 62, 10, 7, 190, 340, color, TFT_BLACK);
    canvas.drawArc(centerX + 14, eyeY - 62, 10, 7, 200, 350, color, TFT_BLACK);
  }
  if (hasOverlay(expr, face::Overlay::GameOver))
    canvas.drawString("GAME OVER", centerX - 56, mouthY + 18, 2);
  if (hasOverlay(expr, face::Overlay::QueasyLines))
  {
    for (int i = 0; i < 3; ++i)
      drawArcStroke(canvas, centerX - 84 + i * 12, mouthY + 10, 6, 4, 200, 360, color, 2);
  }
}

template <typename Canvas>
void drawOLEDStyleEye(Canvas &canvas, int x, int centerY, face::Eye eye, uint16_t color)
{
  if (eye.kind == face::EyeKind::None)
    return;

  int eyeW = (int)(eye.w * 1.9f * eye.sx);
  int eyeH = (int)(eye.h * 1.9f * eye.sy);
  if (eyeW < 4)
    eyeW = 4;
  if (eyeH < 4)
    eyeH = 4;
  const int thick = eye.t > 1 ? (int)eye.t : 1;
  const int rr = eye.r > 0 ? (int)(eye.r * 1.5f) : ((eyeW < eyeH ? eyeW : eyeH) / 2);
  const bool filled = eye.t >= (eye.w < eye.h ? eye.w : eye.h) * 0.49f;

  if (eye.kind == face::EyeKind::Star || eye.kind == face::EyeKind::Sparkle)
  {
    const int r1 = eyeW / 2;
    const int r2 = r1 / 2;
    for (int i = 0; i < 5; ++i)
    {
      const float a0 = (-90.0f + i * 72.0f) * PI / 180.0f;
      const float a1 = (-54.0f + i * 72.0f) * PI / 180.0f;
      int x0 = x + (int)(cosf(a0) * r1);
      int y0 = centerY + (int)(sinf(a0) * r1);
      int x1 = x + (int)(cosf(a1) * r2);
      int y1 = centerY + (int)(sinf(a1) * r2);
      int x2 = x + (int)(cosf(a0 + 72.0f * PI / 180.0f) * r1);
      int y2 = centerY + (int)(sinf(a0 + 72.0f * PI / 180.0f) * r1);
      canvas.drawTriangle(x0, y0, x1, y1, x2, y2, color);
    }
    return;
  }

  if (eye.kind == face::EyeKind::Heart)
  {
    int s = eyeW / 2;
    canvas.fillCircle(x - s / 2, centerY - s / 4, s / 2, color);
    canvas.fillCircle(x + s / 2, centerY - s / 4, s / 2, color);
    canvas.fillTriangle(x - s, centerY, x + s, centerY, x, centerY + s, color);
    if (!filled)
    {
      canvas.fillCircle(x - s / 2, centerY - s / 4, s / 2 - thick, TFT_BLACK);
      canvas.fillCircle(x + s / 2, centerY - s / 4, s / 2 - thick, TFT_BLACK);
    }
    return;
  }

  if (eye.kind == face::EyeKind::Spiral)
  {
    const int loops = 24;
    int px = x;
    int py = centerY;
    for (int i = 1; i <= loops; ++i)
    {
      float u = (float)i / (float)loops;
      float a = u * 6.0f * PI;
      float r = u * (eyeW / 2.0f);
      int nx = x + (int)(cosf(a) * r);
      int ny = centerY + (int)(sinf(a) * r);
      canvas.drawLine(px, py, nx, ny, color);
      px = nx;
      py = ny;
    }
    return;
  }

  if (eye.kind == face::EyeKind::Cross)
  {
    int crossSize = eyeW / 2;
    canvas.drawLine(x - crossSize, centerY - crossSize, x + crossSize, centerY + crossSize, color);
    canvas.drawLine(x + crossSize, centerY - crossSize, x - crossSize, centerY + crossSize, color);
    return;
  }

  if (eye.kind == face::EyeKind::Dollar)
  {
    canvas.drawString("$", x - 6, centerY - 10, 2);
    return;
  }

  if (eye.kind == face::EyeKind::Chevron)
  {
    int hw = eyeW / 2;
    int hh = eyeH / 2;
    canvas.drawLine(x - hw, centerY + hh / 2, x, centerY - hh / 2, color);
    canvas.drawLine(x, centerY - hh / 2, x + hw, centerY + hh / 2, color);
    return;
  }

  if (eye.kind == face::EyeKind::ArcUp)
  {
    drawArcStroke(canvas, x, centerY + 4, eyeW / 2, eyeH / 2, 200, 340, color, thick);
    return;
  }
  if (eye.kind == face::EyeKind::ArcDown)
  {
    drawArcStroke(canvas, x, centerY - 4, eyeW / 2, eyeH / 2, 20, 160, color, thick);
    return;
  }

  if (filled)
  {
    canvas.fillRoundRect(x - eyeW / 2, centerY - eyeH / 2, eyeW, eyeH, rr, color);
    if (eye.pr > 0)
      canvas.fillCircle(x + (int)(eye.px * 2.0f), centerY + (int)(eye.py * 2.0f), (int)(eye.pr * 0.9f), TFT_BLACK);
    if (eye.gr > 0)
      canvas.fillCircle(x + (int)(eye.gx * 2.0f), centerY + (int)(eye.gy * 2.0f), (int)(eye.gr * 0.9f), TFT_BLACK);
  }
  else
  {
    for (int i = 0; i < thick; ++i)
      canvas.drawRoundRect(x - eyeW / 2 + i, centerY - eyeH / 2 + i, eyeW - i * 2, eyeH - i * 2, rr, color);
    if (eye.pr > 0)
      canvas.fillCircle(x + (int)(eye.px * 2.0f), centerY + (int)(eye.py * 2.0f), (int)(eye.pr * 1.0f), color);
  }

  if (eye.lid_cov > 0.0f)
  {
    int lidH = (int)(eyeH * eye.lid_cov);
    canvas.fillRect(x - eyeW / 2 - 2, centerY - eyeH / 2 - 2, eyeW + 4, lidH + 2, TFT_BLACK);
  }
  if (eye.low_cov > 0.0f)
  {
    int lowH = (int)(eyeH * eye.low_cov);
    canvas.fillRect(x - eyeW / 2 - 2, centerY + eyeH / 2 - lowH, eyeW + 4, lowH + 2, TFT_BLACK);
  }
}

template <typename Canvas>
void drawParsedFace(Canvas &canvas, const face::Expression &expr)
{
  const float t = millis() * 0.001f;
  const float blink = blinkStrength(expr, t);
  const float acc = accentStrength(expr, t);

  int centerX = kCenterX;
  int centerY = kCenterY + (int)((expr.lay.eye_y - 27.0f) * 2.0f);
  uint16_t color = TFT_WHITE;

  // Базовая микро-анимация, чтобы даже статичные пресеты выглядели живыми.
  centerY += (int)(sinf(t * 1.2f) * 1.0f);

  // Чтение физики Motion из оригинальных параметров
  const float amp = expr.motion_amp > 0 ? expr.motion_amp : 1.0f;
  if (expr.motion == face::Motion::Pulse)
  {
    float scale = 1.0f + sinf(t * (expr.motion_hz > 0 ? expr.motion_hz : 1.8f) * 2.0f * PI) * 0.08f;
    centerX *= scale;
    centerY *= scale;
  }
  if (expr.motion == face::Motion::Tremble || expr.motion == face::Motion::Shiver)
  {
    centerX += (int)(sinf(t * 55.0f) * amp);
    centerY += (int)(cosf(t * 43.0f) * amp);
  }
  if (expr.motion == face::Motion::Wobble)
  {
    centerX += (int)(sinf(t * (expr.motion_hz > 0 ? expr.motion_hz : 0.8f) * 2.0f * PI) * amp * 1.2f);
    centerY += (int)(sinf(t * (expr.motion_hz > 0 ? expr.motion_hz : 0.8f) * 2.0f * PI + 1.0f) * amp * 0.7f);
  }
  if (expr.motion == face::Motion::Bounce)
    centerY += (int)(fabsf(sinf(t * (expr.motion_hz > 0 ? expr.motion_hz : 2.0f) * PI)) * -2.0f * amp);
  if (expr.motion == face::Motion::Bob || expr.motion == face::Motion::Float)
    centerY += (int)(sinf(t * (expr.motion_hz > 0 ? expr.motion_hz : 0.7f) * 2.0f * PI) * amp);
  if (expr.motion == face::Motion::Sway)
    centerX += (int)(sinf(t * (expr.motion_hz > 0 ? expr.motion_hz : 0.7f) * 2.0f * PI) * amp * 1.5f);
  if (expr.motion == face::Motion::Nod)
    centerY += (int)(sinf(t * (expr.motion_hz > 0 ? expr.motion_hz : 1.4f) * 2.0f * PI) * amp * 1.2f);
  if (expr.motion == face::Motion::Shake)
    centerX += (int)(sinf(t * (expr.motion_hz > 0 ? expr.motion_hz : 4.0f) * 2.0f * PI) * amp * 1.6f);

  int gazeX = 0;
  int gazeY = 0;
  switch (expr.gaze)
  {
  case face::Gaze::Still:
    break;
  case face::Gaze::Around:
    gazeX = (int)(sinf(t * 0.9f) * 3.0f);
    gazeY = (int)(cosf(t * 1.3f) * 2.0f);
    break;
  case face::Gaze::Up:
    gazeY = -3;
    break;
  case face::Gaze::Down:
    gazeY = 3;
    break;
  case face::Gaze::Left:
    gazeX = -3;
    break;
  case face::Gaze::Right:
    gazeX = 3;
    break;
  case face::Gaze::Avert:
    gazeX = -2;
    gazeY = 2;
    break;
  case face::Gaze::Dart:
    gazeX = (int)(sinf(t * 4.5f) * 4.0f);
    gazeY = (int)(cosf(t * 3.9f) * 3.0f);
    break;
  case face::Gaze::UpSide:
    gazeX = 2;
    gazeY = -2;
    break;
  case face::Gaze::Idle:
  default:
    gazeX = (int)(sinf(t * 0.7f) * 2.0f);
    gazeY = (int)(cosf(t * 0.5f) * 1.0f);
    break;
  }

  face::Eye lEye = expr.eyes.l;
  face::Eye rEye = expr.eyes.r;
  lEye.px += gazeX;
  lEye.py += gazeY;
  rEye.px += gazeX;
  rEye.py += gazeY;

  if (blink > 0.0f)
  {
    lEye.lid_cov = lEye.lid_cov < blink ? blink : lEye.lid_cov;
    rEye.lid_cov = rEye.lid_cov < blink ? blink : rEye.lid_cov;
  }

  // Акцент: мягкие периодические жесты (вместо статичной позы).
  if (acc > 0.0f)
  {
    switch (expr.accent)
    {
    case face::Accent::Hop:
      centerY -= (int)(acc * 3.0f);
      break;
    case face::Accent::Tilt:
    case face::Accent::Look:
      lEye.px -= (int)(acc * 2.0f);
      rEye.px += (int)(acc * 2.0f);
      break;
    case face::Accent::Squint:
      lEye.lid_cov = lEye.lid_cov < 0.6f ? 0.6f : lEye.lid_cov;
      rEye.lid_cov = rEye.lid_cov < 0.6f ? 0.6f : rEye.lid_cov;
      break;
    case face::Accent::Nod:
    case face::Accent::Bob:
      centerY += (int)(sinf(t * 8.0f) * 2.0f * acc);
      break;
    case face::Accent::Shake:
    case face::Accent::Wiggle:
    case face::Accent::Twitch:
      centerX += (int)(sinf(t * 30.0f) * acc * 2.0f);
      break;
    case face::Accent::Pulse:
    case face::Accent::Glint:
    case face::Accent::Grin:
    case face::Accent::Sway:
    case face::Accent::Chatter:
    case face::Accent::Brows:
    case face::Accent::Breath:
    case face::Accent::None:
      break;
    }
  }

  const int eyeOffsetX = (int)(expr.lay.spacing > 0 ? expr.lay.spacing : 54) * 2 / 2;
  const int headTilt = (int)(expr.lay.tilt * 2.0f);

  // Рисуем пару глаз (Левый и Правый) из структуры
  drawOLEDStyleEye(canvas, centerX - eyeOffsetX + (int)(lEye.dx * 2.0f), centerY - headTilt + (int)(lEye.dy * 2.0f), lEye, color);
  drawOLEDStyleEye(canvas, centerX + eyeOffsetX - (int)(rEye.dx * 2.0f), centerY + headTilt + (int)(rEye.dy * 2.0f), rEye, color);

  // Отрисовка бровей (Brows)
  if (expr.brows.l.len > 0)
  {
    int bY = centerY - 32 + expr.brows.l.gap;
    canvas.drawLine(centerX - 75, bY, centerX - 35, bY + (expr.brows.l.ang * 0.5), color);
    canvas.drawLine(centerX + 75, bY, centerX + 35, bY + (expr.brows.r.ang * 0.5), color);
  }

  // Отрисовка рта на базе MouthKind
  int mouthY = kMouthBaseY + (int)((expr.lay.mouth_y - 49.0f) * 2.0f) + (int)(expr.mouth.dy * 2.0f);
  mouthY += (int)(sinf(t * 1.4f) * 0.6f);

  if (expr.accent == face::Accent::Chatter && acc > 0.0f)
    mouthY += (int)(sinf(t * 28.0f) * 2.0f * acc);

  int mouthW = expr.mouth.w * 1.8f;

  if (expr.mouth.kind == face::MouthKind::Curve)
  {
    if (expr.mouth.open > 0)
    { // Открытый рот
      canvas.fillEllipse(centerX, mouthY, mouthW / 2, expr.mouth.open * 1.2, color);
      canvas.fillEllipse(centerX, mouthY, mouthW / 2 - 3, expr.mouth.open * 1.2 - 3, TFT_BLACK);
    }
    else
    { // Линия или улыбка
      for (int i = -mouthW / 2; i <= mouthW / 2; i++)
      {
        int h = (expr.mouth.curve != 0) ? (i * i) / 35 : 0;
        canvas.fillRect(centerX + i, mouthY + h, 3, 3, color);
      }
    }
  }
  else if (expr.mouth.kind == face::MouthKind::None)
  {
    // no-op
  }
  else if (expr.mouth.kind == face::MouthKind::Ring)
  {
    int mh = expr.mouth.h > 0 ? (int)(expr.mouth.h * 1.4f) : 10;
    canvas.drawEllipse(centerX, mouthY, mouthW / 2, mh, color);
  }
  else if (expr.mouth.kind == face::MouthKind::Wavy)
  {
    int amp = expr.mouth.h > 0 ? (int)(expr.mouth.h * 1.2f) : 3;
    for (int i = -mouthW / 2; i < mouthW / 2; ++i)
    {
      int y0 = mouthY + (int)(sinf((float)i * 0.25f) * amp);
      int y1 = mouthY + (int)(sinf((float)(i + 1) * 0.25f) * amp);
      canvas.drawLine(centerX + i, y0, centerX + i + 1, y1, color);
    }
  }
  else if (expr.mouth.kind == face::MouthKind::Cat)
  {
    drawArcStroke(canvas, centerX - mouthW / 4, mouthY, mouthW / 4, 8, 20, 160, color, 2);
    drawArcStroke(canvas, centerX + mouthW / 4, mouthY, mouthW / 4, 8, 20, 160, color, 2);
  }
  else if (expr.mouth.kind == face::MouthKind::Pucker)
  {
    int mh = expr.mouth.h > 0 ? (int)(expr.mouth.h * 1.2f) : 12;
    canvas.drawCircle(centerX - 4, mouthY, mh / 3, color);
    canvas.drawCircle(centerX + 4, mouthY, mh / 3, color);
  }
  else if (expr.mouth.kind == face::MouthKind::Teeth || expr.mouth.kind == face::MouthKind::Grin)
  {
    canvas.drawRect(centerX - mouthW / 2, mouthY - 5, mouthW, 12, color);
    if (expr.mouth.kind == face::MouthKind::Grin)
      canvas.drawLine(centerX - mouthW / 2, mouthY, centerX + mouthW / 2, mouthY, color);
  }
  else if (expr.mouth.kind == face::MouthKind::Tongue)
  {
    canvas.drawArc(centerX, mouthY - 4, mouthW / 2, 10, 200, 340, color, TFT_BLACK);
    canvas.drawArc(centerX, mouthY + 2, mouthW / 3, 8, 20, 160, color, TFT_BLACK);
  }
  else if (expr.mouth.kind == face::MouthKind::Triangle)
  {
    int mh = expr.mouth.h > 0 ? (int)(expr.mouth.h * 1.4f) : 12;
    canvas.drawTriangle(centerX - mouthW / 2, mouthY - 4, centerX + mouthW / 2, mouthY - 4, centerX, mouthY + mh, color);
  }
  else if (expr.mouth.kind == face::MouthKind::Scream)
  {
    int mh = expr.mouth.open > 0 ? (int)(expr.mouth.open * 1.4f) : 12;
    canvas.drawRoundRect(centerX - mouthW / 2, mouthY - mh / 2, mouthW, mh, 6, color);
    int teeth = expr.mouth.h > 0 ? (int)expr.mouth.h : 3;
    canvas.drawLine(centerX - mouthW / 2 + 2, mouthY - mh / 2 + teeth, centerX + mouthW / 2 - 2, mouthY - mh / 2 + teeth, color);
  }
  else if (expr.mouth.kind == face::MouthKind::Rage)
  {
    int mh = expr.mouth.open > 0 ? (int)(expr.mouth.open * 1.4f) : 12;
    canvas.drawRoundRect(centerX - mouthW / 2, mouthY - mh / 2, mouthW, mh, 6, color);
    int teeth = expr.mouth.h > 0 ? (int)expr.mouth.h : 3;
    canvas.drawLine(centerX - mouthW / 2 + 2, mouthY - mh / 2 + teeth, centerX + mouthW / 2 - 2, mouthY - mh / 2 + teeth, color);
    canvas.drawLine(centerX - mouthW / 2 + 2, mouthY + mh / 2 - teeth, centerX + mouthW / 2 - 2, mouthY + mh / 2 - teeth, color);
  }
  else if (expr.mouth.kind == face::MouthKind::Box)
  {
    int mh = expr.mouth.h > 0 ? (int)(expr.mouth.h * 1.4f) : 12;
    canvas.drawRect(centerX - mouthW / 2, mouthY - mh / 2, mouthW, mh, color);
  }

  drawOverlays(canvas, expr, centerX, centerY, mouthY, color, t);
}

void drawBootTest()
{
  tft.fillScreen(TFT_BLACK);
  tft.drawCircle(kCenterX, kCenterY, 40, TFT_DARKGREY);
  tft.fillCircle(kCenterX, kCenterY, 6, TFT_GREEN);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("IPS 240x240 OK", 60, 12, 2);
  delay(900);
}

void setup()
{
  randomSeed((uint32_t)micros());

  pinMode(PHOTORESISTOR_PIN, INPUT);
  const int level = digitalRead(PHOTORESISTOR_PIN);
  lightRawDark = kDarkWhenLow ? (level == LOW) : (level == HIGH);
  lightStableDark = lightRawDark;
  lightLastEdgeAt = millis();

  tft.init();
  tft.setRotation(0);
  tft.invertDisplay(kInvertDisplay);
  tft.fillScreen(TFT_BLACK);

  drawBootTest();

  // 8-bit sprite significantly lowers RAM usage and helps avoid fallback flicker.
  sprite.setColorDepth(8);
  useSprite = sprite.createSprite(kScreenW, kScreenH) != nullptr;

  // Если full-screen sprite не выделился, рисуем напрямую в TFT.
  if (!useSprite)
  {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("Sprite fallback", 64, 222, 2);
  }

  const uint32_t now = millis();
  setExpression(lightStableDark ? face::Expr::sleepy : face::Expr::neutral, now);
}

void loop()
{
  unsigned long currentTime = millis();
  mouthWave += 0.08;

  const bool changed = updateLightState(currentTime);
  if (changed)
  {
    if (lightStableDark)
    {
      setExpression(face::Expr::sleepy, currentTime);
    }
    else
    {
      setExpression(face::Expr::waking, currentTime);
    }
  }

  if (lightStableDark)
  {
    if (currentTime >= nextEmotionAt)
    {
      if (currentExpr == face::Expr::sleepy)
        setExpression(face::Expr::sleeping, currentTime);
      else
        setExpression(face::Expr::sleeping, currentTime);
    }
  }
  else
  {
    advanceBehavior(currentTime);
  }

  const face::Expression &expr = face::expression(currentExpr);

  if (useSprite)
  {
    sprite.fillSprite(TFT_BLACK);
    drawParsedFace(sprite, expr);
    sprite.pushSprite(0, 0);
  }
  else
  {
    tft.fillScreen(TFT_BLACK);
    drawParsedFace(tft, expr);
  }

  delay(fpsInterval);
}
