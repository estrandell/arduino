/*
 * 
 * http://www.alanzucconi.com/2016/01/06/colour-interpolation/2/
 * https://gist.github.com/cjddmut/fefe5dac35cccfceabec
 * 
 * 
 * 
 */

inline float Clamp01(float a){
  return min(max(0.0f,a),1.0f);
}

#define EPSILON 0.000001

#define HSV_RED				HSV(         0, 1.0, 1.0, 1.0)
#define HSV_ORANGE			HSV( 30/360.0f, 1.0, 1.0, 1.0)
#define HSV_YELLOW			HSV( 60/360.0f, 1.0, 1.0, 1.0)
#define HSV_YELLOW_GREEN	HSV( 90/360.0f, 1.0, 1.0, 1.0)
#define HSV_GREEN			HSV(120/360.0f, 1.0, 1.0, 1.0)
#define HSV_GREEN_CYAN		HSV(150/360.0f, 1.0, 1.0, 1.0)
#define HSV_CYAN			HSV(180/360.0f, 1.0, 1.0, 1.0)
#define HSV_BLUE_CYAN		HSV(210/360.0f, 1.0, 1.0, 1.0)
#define HSV_BLUE			HSV(240/360.0f, 1.0, 1.0, 1.0)
#define HSV_VIOLET			HSV(270/360.0f, 1.0, 1.0, 1.0)
#define HSV_MAGENTA			HSV(300/360.0f, 1.0, 1.0, 1.0)
#define HSV_RED_MAGENTA		HSV(330/360.0f, 1.0, 1.0, 1.0)


struct Vec3
{
    float x;
    float y;
    float z;

    public : Vec3(){
      x = 0.0f;
      y = 0.0f;
      z = 0.0f;
    }

    public : Vec3(float x, float y, float z)
    {
      this->x = x;
      this->y = y;
      this->z = z;
    }

    Vec3 operator+(const Vec3& a) const
    {
        return Vec3(x+a.x, y+a.y, z+a.z);
    }
    Vec3 operator-(const Vec3& a) const
    {
        return Vec3(x-a.x, y-a.y, z-a.z);
    }
    Vec3 operator*(const float a) const
    {
        return Vec3(a*x, a*y, a*z);
    }
};


struct Vec4
{
    float x;
    float y;
    float z;
    float w;

    public : Vec4(){
      x = 0.0f;
      y = 0.0f;
      z = 0.0f;
      w = 0.0f;
    }

    public : Vec4(float x, float y, float z, float w)
    {
      this->x = x;
      this->y = y;
      this->z = z;
      this->w = w;
    }
};


struct RGBA
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    public : RGBA()
    {
        this->r = 0;
        this->g = 0;
        this->b = 0;
        this->a = 0;
    }
    public : RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        this->r = r;
        this->g = g;
        this->b = b;
        this->a = a;
    }
    public : RGBA(uint32_t c)
    {
        r = (uint8_t)(c >> 16);
        g = (uint8_t)(c >>  8);
        b = (uint8_t)(c >>  0);
        a = 255;
    }

    public : uint32_t ToUInt32(){
        return (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
    }


    public : void Print(){
        Serial.print(r);
        Serial.print(" ");
        Serial.print(g);
        Serial.print(" ");
        Serial.print(b);
        Serial.print(" ");
        Serial.println(a);
    }
};


struct HSV
{
    float h;
    float s;
    float v;
    float a;

    public : HSV()
    {
        this->h = 0.0f;
        this->s = 0.0f;
        this->v = 0.0f;
        this->a = 0.0f;
    }
	
    public : HSV(float h, float s, float v, float a)
    {
        this->h = h;
        this->s = s;
        this->v = v;
        this->a = a;
    }

    public : RGBA ToRgb(){
        float r = Clamp01(abs(h * 6.0f - 3.0f) - 1.0f);
        float g = Clamp01(2.0 - abs(h * 6.0f - 2.0f));
        float b = Clamp01(2.0 - abs(h * 6.0f - 4.0f));

        Vec3 rgb = Vec3(r, g, b);
        static const Vec3 one(1.0f, 1.0f, 1.0f);
        Vec3 vc = ((rgb - one) * s + one) * v;

        return RGBA(vc.x*255, vc.y*255, vc.z*255, a*255);
    }

    public : void Print(){
        Serial.print(h);
        Serial.print(" ");
        Serial.print(s);
        Serial.print(" ");
        Serial.print(v);
        Serial.print(" ");
        Serial.println(a);
    }
};


static HSV Rgb2Hsv(RGBA & rgb){

        float r = rgb.r / 255.0f;
        float g = rgb.g / 255.0f;
        float b = rgb.b / 255.0f;
        float a = rgb.a / 255.0f;
        
        Vec4 p = g < b   ? Vec4(b, g, -1.0f, 2.0f/3.0f) : Vec4(g, b, 0.0f, -1.0f/3.0f);
        Vec4 q = r < p.x ? Vec4(p.x, p.y, p.w, r)    : Vec4(r, p.y, p.z, p.x);
        float c = q.x - min(q.w, q.y);
        float h = abs((q.w - q.y) / (6.0f * c + EPSILON) + q.z);
        
        Vec3 hcv = Vec3(h, c, q.x);
        float s = hcv.y / (hcv.z + EPSILON);

        return HSV(hcv.x, s, hcv.z, a);
}

static RGBA Hsv2Rgb(HSV & hsv){

        float r = Clamp01(abs(hsv.h * 6.0f - 3.0f) - 1.0f);
        float g = Clamp01(2.0f - abs(hsv.h * 6.0f - 2.0f));
        float b = Clamp01(2.0f - abs(hsv.h * 6.0f - 4.0f));

        Vec3 rgb = Vec3(r, g, b);
        Vec3 one(1.0f,1.0f,1.0f);
        Vec3 vc = ((rgb - one) * hsv.s + one) * hsv.v;

        return RGBA(vc.x*255, vc.y*255, vc.z*255, hsv.a*255);
}

static HSV LerpHSV (HSV a, HSV b, float t)
{
  // Hue interpolation
  float h;
  float d = b.h - a.h;
  if (a.h > b.h)
  {
    // Swap (a.h, b.h)
    float h3 = b.h;
    b.h = a.h;
    a.h = h3;
 
    d = -d;
    t = 1.0f - t;
  }
 
  if (d > 0.5f) // 180deg
  {
    a.h = fmod(a.h + 1.0f, 1.0f); // 360deg
    h = ( a.h + t * (b.h - a.h) ); // 360deg
  }
  else // if (d <= 0.5f) // 180deg
  {
    h = a.h + t * d;
  }
 
  // Interpolates the rest
  return HSV
  (
    h,      // H
    a.s + t * (b.s-a.s),  // S
    a.v + t * (b.v-a.v),  // V
    a.v + t * (b.v-a.v)   // A
  );
}


static HSV HSVClock(int hour){
  switch (hour)
  {
    case  0: return HSV(         0, 1.0, 1.0, 1.0); //red
    case  1: return HSV( 30/360.0f, 1.0, 1.0, 1.0); //orange
    case  2: return HSV( 60/360.0f, 1.0, 1.0, 1.0); //yellow
    case  3: return HSV( 90/360.0f, 1.0, 1.0, 1.0); //yellow green
    case  4: return HSV(120/360.0f, 1.0, 1.0, 1.0); //green
    case  5: return HSV(150/360.0f, 1.0, 1.0, 1.0); //green cyan
    case  6: return HSV(180/360.0f, 1.0, 1.0, 1.0); //cyan
    case  7: return HSV(210/360.0f, 1.0, 1.0, 1.0); //blue cyan
    case  8: return HSV(240/360.0f, 1.0, 1.0, 1.0); //blue
    case  9: return HSV(270/360.0f, 1.0, 1.0, 1.0); //violet
    case 10: return HSV(300/360.0f, 1.0, 1.0, 1.0); //magenta
    case 11: return HSV(330/360.0f, 1.0, 1.0, 1.0); //red magenta
  }
}

static HSV HSVClockRandom(){
	return HSVClock(random(12));
}

