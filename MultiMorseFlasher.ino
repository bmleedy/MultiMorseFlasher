/*
 * Flashing morse code LEDs.
 */

// Use N-millisecond time divisions, 
// flashing one division for a dot and three divisions for a dash
// https://en.wikipedia.org/wiki/Morse_code
// Example "A":
// |   |   |   |   |   |   |   |   |   |
// ****    ************
// I need a word, which I can iterate through


#define TIME_DIVISION_MILLIS 500  //todo: this is super slow
#define VERBOSE false

struct code
{
  unsigned char bits;
  unsigned char len;
};

/*
 * -----------------------------------------------------------------
 * class MorseAlphabet
 * 
 * Below, I define a constant array mapping morse codes to ascii
 * characters.  The MorseAlphabet class accesses this array safely
 * with input validations.
 * -----------------------------------------------------------------
 */
#define ASCII_FIRST_CHARACTER_OFFSET 32

const code ascii_to_morse[] = { // right to left bit encoded, 1=-; 0=. , only len rightmost bits valid
  /*   = " "       */ { 0b000000, 0},
  /* ! = "-.-.--"  */ { 0b110101, 6},
  /* " = ".-..-."  */ { 0b010010, 6},
  /* # = ""        */ { 0b111111, 0},
  /* $ = ".-..."   */ { 0b000010, 5},
  /* % = ""        */ { 0b111111, 0},
  /* & = ""        */ { 0b111111, 0}, 
  /* ' = ".----."  */ { 0b011110, 6},
  /* ( = "-.--."   */ { 0b001101, 5},
  /* ) = "-.--.-"  */ { 0b101101, 6},
  /* * = ""        */ { 0b111111, 0},
  /* + = ".-.-."   */ { 0b001010, 5},
  /* , = "--..--"  */ { 0b110011, 6},
  /* - = "-....-"  */ { 0b100001, 6},
  /* . = ".-.-.-"  */ { 0b101010, 6},
  /* / = "-..-."   */ { 0b001001, 5},
  /* 0 = "-----"   */ { 0b011111, 5},
  /* 1 = ".----"   */ { 0b011110, 5},
  /* 2 = "..---"   */ { 0b011100, 5},
  /* 3 = "...--"   */ { 0b011000, 5},
  /* 4 = "....-"   */ { 0b010000, 5},
  /* 5 = "....."   */ { 0b000000, 5},
  /* 6 = "-...."   */ { 0b000001, 5},
  /* 7 = "--..."   */ { 0b000011, 5},
  /* 8 = "---.."   */ { 0b000111, 5},
  /* 9 = "----."   */ { 0b001111, 5},
  /* : = "---..."  */ { 0b000111, 6},
  /* ; = "-.-.-."  */ { 0b010101, 6},
  /* < = ""        */ { 0b111111, 0},
  /* = = "-...-"   */ { 0b010001, 5},
  /* > = ""        */ { 0b111111, 0},
  /* ? = "..--.."  */ { 0b001100, 6},
  /* @ = ".--.-."  */ { 0b010110, 6},
  /* A = ".-"      */ { 0b000010, 2},
  /* B = "-..."    */ { 0b000001, 4},
  /* C = "-.-."    */ { 0b000101, 4},
  /* D = "-.."     */ { 0b000001, 3},
  /* E = "."       */ { 0b000000, 1},
  /* F = "..-."    */ { 0b000100, 4},
  /* G = "--."     */ { 0b000011, 3},
  /* H = "...."    */ { 0b000000, 4},
  /* I = ".."      */ { 0b000000, 2},
  /* J = ".---"    */ { 0b001110, 4},
  /* K = "-.-"     */ { 0b000101, 3},
  /* L = ".-.."    */ { 0b000010, 4},
  /* M = "--"      */ { 0b000011, 2},
  /* N = "-."      */ { 0b000001, 2},
  /* O = "---"     */ { 0b000111, 3},
  /* P = ".--."    */ { 0b000110, 4},
  /* Q = "--.-"    */ { 0b001011, 4},
  /* R = ".-."     */ { 0b000010, 3},
  /* S = "..."     */ { 0b000000, 3},
  /* T = "-"       */ { 0b000001, 1},
  /* U = "..-"     */ { 0b000100, 3},
  /* V = "...-"    */ { 0b001000, 4},
  /* W = ".--"     */ { 0b000110, 3},
  /* X = "-..-"    */ { 0b001001, 4},
  /* Y = "-.--"    */ { 0b001101, 4},
  /* Z = "--.."    */ { 0b000011, 4}
  };


class MorseAlphabet
{
  char code_list_length = 59;  //length of array defined above


  unsigned int get_code_id(char letter)
  {
    unsigned int rv;
    // Only allow upper case letters
    if(isUpperCase(letter))
    {
      rv = letter - ASCII_FIRST_CHARACTER_OFFSET;
    }
    else
    {
      Serial.println("Out of bounds character detected: "); Serial.println(letter);
      rv = 0;  //return space if out of bounds
    }

    return rv;
  }


  public:
  unsigned char get_code_len(char letter)
  {
    unsigned int code_id = get_code_id(letter);
    return ascii_to_morse[code_id].len;
  }
  
  unsigned char get_code_bits(char letter)
  {
    unsigned int code_id = get_code_id(letter);
    return ascii_to_morse[code_id].bits;
  }

  // get the bit at a specific offset in a char
  bool get_code_bit(char letter, int bit_offset)
  {
    if(bit_offset >= 8 || bit_offset < 0)
    {
      //input is out of bounds
      Serial.print("WARNING: Bad bit offset requested: ");Serial.println(bit_offset);
      return false;
    }
    unsigned int bits = this->get_code_bits(letter);
    bool rv = (bits << bit_offset) & 1;
    return rv;
  }

  // just a more readable wrapper for get_code_bit
  bool is_dash(char letter, int bit_offset)
  {
    return get_code_bit(letter, bit_offset);
  }
};




/*
 * -----------------------------------------------------------------
 * class WordFlasher
 * 
 * This class will flash words in morse code using the MorseAlphabet
 *  class.  
 *  
 * Call the run_flash() method once every time division to set the 
 *   state of the selected digital output.
 * -----------------------------------------------------------------
 */
MorseAlphabet alpha;
 
class WordFlasher
{
private:
//todo: move all of this to header
  enum blinkstate {
    notstarted,
    dotting,
    dashing,
    spacing,
    ofsetting
  } blink_state;

  unsigned int len;        //the length of the word
  char * word_string;      //the word to blink
  unsigned int output_pin; //this is the pin which will blink (class will set up the output mode in constructor)
  int time_div_countdown, code_offset, bit_offset;  //counters for iteration

public:
  //Constructor - instantiate a word, like "EVAN"
  WordFlasher(const char* word_string, unsigned int len, int output_pin)
  {
    this->time_div_countdown = 0;
    this->word_string = (char*)word_string;
    this->len = len;
    this->output_pin = output_pin;

    pinMode(this->output_pin, OUTPUT);
    this->reset();
  }

private:
  void reset()
  {
    this->code_offset = 0; //how many codes have we completed
    this->bit_offset = 0;  //how many bits have we completed
    this->blink_state = notstarted; //same as space, ready to transition to a dot or dash immediately.
    digitalWrite(this->output_pin, HIGH);
  }

  void space(int num_time_divisions)
  {
    blink_state = spacing;
    digitalWrite(this->output_pin, LOW);
    time_div_countdown = num_time_divisions;
  }

  void dot()
  {
    blink_state=dotting;
    time_div_countdown = 1;
    digitalWrite(this->output_pin, HIGH);
  }

  void dash()
  {
    blink_state=dashing;
    time_div_countdown = 3;
    digitalWrite(this->output_pin, HIGH);
  }

  void word_offset()
  {
    blink_state=ofsetting;
    time_div_countdown=7;
    digitalWrite(this->output_pin, LOW);
  }
  
public:
  void runflash()
  {
    if(VERBOSE){Serial.println(time_div_countdown);}
    if(time_div_countdown-- <= 0)  //only take new action if we're done with our previous action
      switch (blink_state) {
        case notstarted:
        case spacing:
          if(VERBOSE){Serial.println("spacing");}
          // Lookup whether the next bit I want to read is beyond the length of the code I'm currently playing.
          if(this->bit_offset >= alpha.get_code_len(word_string[this->code_offset]))
          {
            //no dots or dashes left we need a new code
            //  I have just run out of bits, 
            //   reset the bit offset and increment the letter offset.
            this->code_offset++;  //move to next letter
            Serial.print("new letter: "); Serial.write(this->word_string[this->code_offset]); Serial.println("");
            this->bit_offset = 0; //start at beginning of that next letter
            //   if I have run out of letters, call (offset) which will put me into offsetting state
            if(this->code_offset >= this->len - 1 )  //todo: invert this to make cleaner
            {
              //I have run out of letters call an offset, which will lead to me restarting.
              this->word_offset();
            }
            else
            {
              //put two more time divisions in before starting the next letter, per the morse standard
              space(2);
            }
          }
          else  //I still have dots or dashes to blink, start the next one
          {
            // I have len bits remaining

            // call dot() or dash()
            if( alpha.is_dash(this->word_string[this->code_offset], this->bit_offset) )
              dash();
            else
              dot();
              
            // Shift the bits by one more bit number for the next time around
            this->bit_offset++;
            if(VERBOSE){Serial.print("letter: "); Serial.write(this->word_string[this->code_offset]); Serial.println("");}
            if(VERBOSE){Serial.print("len: "); Serial.println(alpha.get_code_len(word_string[this->code_offset]));}
            if(VERBOSE){Serial.print("binary pattern: "); Serial.println(alpha.get_code_bits(word_string[this->code_offset]),BIN);}
            if(VERBOSE){Serial.print("offsetting bits by "); Serial.println(this->bit_offset);}
          }
          break;
        case dotting:
          if(VERBOSE){Serial.print("dotting");}
        case dashing:
          if(VERBOSE){Serial.println("dashing");}
          this->space(1); //always put a space after a dot or a dash.
          break;
        case ofsetting:
          if(VERBOSE){Serial.println("ofsetting");}
          this->reset();
          this->space(1);
          break;
        default:
          //todo: put a serial println in here to indicate a bug.
          break;
      }
  }
  
};

/*
 * -----------------------------------------------------------------
 * setup()
 * 
 * Creates instances of word flashers for all words I want to flash
 * -----------------------------------------------------------------
 */

WordFlasher *evan;
WordFlasher * cooper;
WordFlasher * oliver;

void setup() {
  // Setup the debug serial port
  Serial.begin(9600);
  
  // put your setup code here, to run once:
  evan   = new WordFlasher("EVAN",4, 6);
  cooper = new WordFlasher("COOPER",6, 4);  //todo: make work for multipe LED's
  oliver = new WordFlasher("OLIVER",6, 5);  //todo: make work for multiple LED's

}

/*
 * -----------------------------------------------------------------
 * loop()
 * 
 * Once set up, I just call runflash() for each of my words that I 
 *   want to blink, once per time division.
 * -----------------------------------------------------------------
 */
void loop() {
  evan->runflash();
  cooper->runflash();
  oliver->runflash();
  
  delay(TIME_DIVISION_MILLIS);

}
