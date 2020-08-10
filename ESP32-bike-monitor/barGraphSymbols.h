// barGraphSymbols.h
// defines four vertical bar symbols for 1 through 4
// use built in characters for 0 and 5 Bars
// define small character for 0, 5, 10, and 15

// LCD Bar graph symbols
const int BAR_0 = 254;  // Blank (zero vertical Bars)
const int BAR_1 = 1;       // one vertical Bar
const int BAR_2 = 2;       // two vertical Bars
const int BAR_3 = 3;       // three vertical Bars
const int BAR_4 = 4;       // four vertical Bars
const int BAR_5 = 255;     // all five vertical Bars
const int NUM_0 = 0;       // number zero
const int NUM_5 = 5;       // number 5
const int NUM_10 = 6;      // number 10
const int NUM_15 = 7;      // number 15

// axis for 16 amp bar graph
char axis16amp[17] =
{
  'A',    ' ', ' ', ' ',
  NUM_5,  ' ', ' ', ' ', ' ',
  NUM_10, ' ', ' ', ' ', ' ',
  NUM_15
};

// axis for 400 watt bar graph
char axis400watt[17] =
{
  'W', ' ', ' ',
  '1', ' ', ' ', ' ',
  '2', ' ', ' ', ' ',
  '3', ' ', ' ', ' ',
  '4'
};

// define 4 arrays that form each vertical bars 1 through 4
// use symbol 255 for all 5 vertical Bars and 254 for no bars
// define 4 arrays for low profile numbers 0, 5, 10, and 15
byte bar1[8] =
{
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000
};

byte bar2[8] =
{
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000
};

byte bar3[8] =
{
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100
};

byte bar4[8] =
{
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110
};

byte num0[8] =
{
  B00100,
  B01010,
  B01010,
  B01010,
  B01010,
  B00100,
  B00000,
  B00000
};

byte num5[8] =
{
  B01110,
  B01000,
  B01100,
  B00010,
  B01010,
  B00100,
  B00000,
  B00000
};

byte num10[8] =
{
  B10010,
  B10101,
  B10101,
  B10101,
  B10101,
  B10010,
  B00000,
  B00000
};

byte num15[8] =
{
  B10111,
  B10100,
  B10110,
  B10001,
  B10101,
  B10010,
  B00000,
  B00000
};
