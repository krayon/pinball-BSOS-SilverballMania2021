/**************************************************************************
    SilverballMania2021 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
*/

// SpotSilverballLetter is broken for last letter


#include "BSOS_Config.h"
#include "BallySternOS.h"
#include "SilverballMania2021.h"
#include "SelfTestAndAudit.h"
#include <EEPROM.h>

// Wav Trigger defines have been moved to BSOS_Config.h

#define USE_SCORE_OVERRIDES

#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
#include "SendOnlyWavTrigger.h"
SendOnlyWavTrigger wTrig;             // Our WAV Trigger object
#endif

#define PINBALL_MACHINE_BASE_MAJOR_VERSION  2021
#define PINBALL_MACHINE_BASE_MINOR_VERSION  1
#define DEBUG_MESSAGES  0



/*********************************************************************

    Game specific code

*********************************************************************/

// MachineState
//  0 - Attract Mode
//  negative - self-test modes
//  positive - game play
char MachineState = 0;
boolean MachineStateChanged = true;
#define MACHINE_STATE_ATTRACT         0
#define MACHINE_STATE_INIT_GAMEPLAY   1
#define MACHINE_STATE_INIT_NEW_BALL   2
#define MACHINE_STATE_NORMAL_GAMEPLAY 4
#define MACHINE_STATE_COUNTDOWN_BONUS 99
#define MACHINE_STATE_BALL_OVER       100
#define MACHINE_STATE_MATCH_MODE      110

#define MACHINE_STATE_ADJUST_FREEPLAY           -17
#define MACHINE_STATE_ADJUST_BALL_SAVE          -18
#define MACHINE_STATE_ADJUST_MUSIC_LEVEL        -19
#define MACHINE_STATE_ADJUST_TOURNAMENT_SCORING -20
#define MACHINE_STATE_ADJUST_TILT_WARNING       -21
#define MACHINE_STATE_ADJUST_AWARD_OVERRIDE     -22
#define MACHINE_STATE_ADJUST_BALLS_OVERRIDE     -23
#define MACHINE_STATE_ADJUST_SCROLLING_SCORES   -24
#define MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD   -25
#define MACHINE_STATE_ADJUST_SPECIAL_AWARD      -26
#define MACHINE_STATE_ADJUST_DIM_LEVEL          -27
#define MACHINE_STATE_ADJUST_DONE               -28

// The lower 4 bits of the Game Mode are modes, the upper 4 are for frenzies
// and other flags that carry through different modes
#define GAME_MODE_SKILL_SHOT                        0
#define GAME_MODE_UNSTRUCTURED_PLAY                 4
#define GAME_MODE_WIZARD_START                      12
#define GAME_MODE_WIZARD                            13
#define GAME_MODE_WIZARD_FINISHED                   14
#define GAME_BASE_MODE                              0x0F



#define EEPROM_BALL_SAVE_BYTE           100
#define EEPROM_FREE_PLAY_BYTE           101
#define EEPROM_MUSIC_LEVEL_BYTE         102
#define EEPROM_SKILL_SHOT_BYTE          103
#define EEPROM_TILT_WARNING_BYTE        104
#define EEPROM_AWARD_OVERRIDE_BYTE      105
#define EEPROM_BALLS_OVERRIDE_BYTE      106
#define EEPROM_TOURNAMENT_SCORING_BYTE  107
#define EEPROM_SCROLLING_SCORES_BYTE    110
#define EEPROM_DIM_LEVEL_BYTE           113
#define EEPROM_EXTRA_BALL_SCORE_BYTE    140
#define EEPROM_SPECIAL_SCORE_BYTE       144


#define SOUND_EFFECT_NONE               0
#define SOUND_EFFECT_BONUS_COUNT_LO     1
#define SOUND_EFFECT_BONUS_COUNT_HI     2
#define SOUND_EFFECT_INLANE_UNLIT       3
#define SOUND_EFFECT_OUTLANE_UNLIT      4
#define SOUND_EFFECT_INLANE_LIT         5
#define SOUND_EFFECT_OUTLANE_LIT        6
#define SOUND_EFFECT_BUMPER_HIT         7
#define SOUND_EFFECT_ADD_CREDIT         10
#define SOUND_EFFECT_LIGHT_LETTER       11
#define SOUND_EFFECT_KICKER_WATCH       12
#define SOUND_EFFECT_KICKER_LAUNCH      13
#define SOUND_EFFECT_BALL_OVER          19
#define SOUND_EFFECT_GAME_OVER          20
#define SOUND_EFFECT_EXTRA_BALL         23
#define SOUND_EFFECT_MACHINE_START      24
#define SOUND_EFFECT_SKILL_SHOT         25
#define SOUND_EFFECT_TILT_WARNING       28
#define SOUND_EFFECT_MATCH_SPIN         30
#define SOUND_EFFECT_SPINNER_HIGH       32
#define SOUND_EFFECT_SPINNER_LOW        33
#define SOUND_EFFECT_SLING_SHOT         34
#define SOUND_EFFECT_ROLLOVER           35
#define SOUND_EFFECT_50PT_SWITCH        36
#define SOUND_EFFECT_HORSESHOE          37
#define SOUND_EFFECT_SILVERBALL_COMPLETION  38
#define SOUND_EFFECT_BONUS_X_INCREASED  39
#define SOUND_EFFECT_ANIMATION_TICK     40
#define SOUND_EFFECT_ADDED_BONUS_QUALIFIED  41
#define SOUND_EFFECT_ADDED_BONUS_COLLECT  42
#define SOUND_EFFECT_ADD_PLAYER_1       50
#define SOUND_EFFECT_ADD_PLAYER_2       (SOUND_EFFECT_ADD_PLAYER_1+1)
#define SOUND_EFFECT_ADD_PLAYER_3       (SOUND_EFFECT_ADD_PLAYER_1+2)
#define SOUND_EFFECT_ADD_PLAYER_4       (SOUND_EFFECT_ADD_PLAYER_1+3)
#define SOUND_EFFECT_PLAYER_1_UP        54
#define SOUND_EFFECT_PLAYER_2_UP        (SOUND_EFFECT_PLAYER_1_UP+1)
#define SOUND_EFFECT_PLAYER_3_UP        (SOUND_EFFECT_PLAYER_1_UP+2)
#define SOUND_EFFECT_PLAYER_4_UP        (SOUND_EFFECT_PLAYER_1_UP+3)
#define SOUND_EFFECT_SHOOT_AGAIN        60
#define SOUND_EFFECT_TILT               61
#define SOUND_EFFECT_VOICE_EXTRA_BALL             81
#define SOUND_EFFECT_WIZARD_MODE_START            88
#define SOUND_EFFECT_WIZARD_MODE_FINISHED         89
#define SOUND_EFFECT_BACKGROUND_1       90
#define SOUND_EFFECT_BACKGROUND_2       91
#define SOUND_EFFECT_BACKGROUND_3       92
#define SOUND_EFFECT_BACKGROUND_WIZ     93

#define MAX_DISPLAY_BONUS     55
#define TILT_WARNING_DEBOUNCE_TIME      1000


/*********************************************************************

    Machine state and options

*********************************************************************/
unsigned long HighScore = 0;
unsigned long AwardScores[3];
byte Credits = 0;
boolean FreePlayMode = false;
byte MusicLevel = 3;
byte BallSaveNumSeconds = 0;
unsigned long ExtraBallValue = 0;
unsigned long SpecialValue = 0;
unsigned long CurrentTime = 0;
byte MaximumCredits = 40;
byte BallsPerGame = 3;
byte DimLevel = 2;
byte ScoreAwardReplay = 0;
boolean HighScoreReplay = true;
boolean MatchFeature = true;
boolean TournamentScoring = false;
boolean ScrollingScores = true;


/*********************************************************************

    Game State

*********************************************************************/
byte CurrentPlayer = 0;
byte CurrentBallInPlay = 1;
byte CurrentNumPlayers = 0;
byte Bonus[4];
byte CurrentBonus;
byte BonusX[4];
byte GameMode = GAME_MODE_SKILL_SHOT;
byte MaxTiltWarnings = 2;
byte NumTiltWarnings = 0;

boolean SamePlayerShootsAgain = false;
boolean BallSaveUsed = false;
boolean CurrentlyShowingBallSave = false;
boolean ExtraBallCollected = false;
boolean SpecialCollected = false;
boolean ShowingModeStats = false;

unsigned long CurrentScores[4];
unsigned long BallFirstSwitchHitTime = 0;
unsigned long BallTimeInTrough = 0;
unsigned long GameModeStartTime = 0;
unsigned long GameModeEndTime = 0;
unsigned long LastTiltWarningTime = 0;
unsigned long ScoreAdditionAnimation;
unsigned long ScoreAdditionAnimationStartTime;
unsigned long LastRemainingAnimatedScoreShown;
unsigned long ScoreMultiplier;



/*********************************************************************

    Game Specific State Variables


byte SilverballStatus[4][15];

Bottom nibble is current playfield status:
Number of times this letter has been collected
Once all letters are equal they can be collected and
the player moves onto the next SilverballMode

Top nibble is current backbox status (for Silverball only):
Number of times this letter has been completed


SILVERBALL_MODE_KNOCK_OUT_LIGHTS
  All standup lights are lit at beginning
  Hitting a standup moves it to playfield lights (playfield light flashes quickly for 2 seconds with 1k for repeat hit)
  Unlit toplane qualifies horseshoe to spot next light (next spotted letter standup flashes slowly)
  Getting an outlane letter activates kicker for 3 seconds
  Getting an inlane letter lights opposite spinner for 3 seconds
  
SILVERBALL_MODE_WORD_GROUPS
  SILVER standup lights are lit
  When those are finished playfield SILVER flashes with 15k reward, BALL lights are lit
  When those are finished playfield BALL flashes with 15k reward, MANIA lights are lit
  Horseshoe qualifies outlane kicker rescue for 30 seconds, then outlane kicker rescue for 3 seconds

SILVERBALL_MODE_FADEAWAY_LETTERS
  All standup lights are lit at beginning
  When any letter is hit (lit or not) it resets the fadeaway timer
  When fadeaway timer gets to 20 seconds, a random letter on playfield blinks slowly
  When fadeaway timer gets to 30 seconds, the letter gets erased and standup is re-lit
  When all letters are done, they are collected to top
  Toplane shifts SILVERBALL left and MANIA right.
  
SILVERBALL_MODE_IN_ORDER


*********************************************************************/
byte TotalSpins[4];
byte LastAwardShotCalloutPlayed;
byte LastWizardTimer;
byte SilverballStatus[4][15];
byte SilverballMode[4];
byte KickerStatus[4];
byte CurrentSilverballWord;
byte SilverballPhase;
byte ToplanePhase;
byte ToplaneProgress;
byte AddedBonusQualified[4];
byte AddedBonusAchieved[4];
byte AlternatingComboPhase;
byte AlternatingCombosHit;
byte Spinner1kPhase;

boolean WizardScoring;
boolean SuperSkillshotQualified;

unsigned long LastInlaneHitTime;
unsigned long BonusXAnimationStart;
unsigned long LastSpinnerHit;
unsigned long SilverballHighlightEnd[15];
unsigned long KickerTimeout;
unsigned long KickerRolloverWatch;
unsigned long ToplaneAnimationEnd;
unsigned long LastSilverballSwitchHit;
unsigned long LastSilverballLetterAchieved;
unsigned long LastHorseshoe;
unsigned long AddedBonusExpiration;
unsigned long AlternatingComboExpiration;


#define SILVERBALL_MODE_KNOCK_OUT_LIGHTS  1
#define SILVERBALL_MODE_WORD_GROUPS       2
#define SILVERBALL_MODE_FADEAWAY_LETTERS  3

#define SILVERBALL_COMPLETION_AWARD       20000

#define WIZARD_START_DURATION             5000
#define WIZARD_DURATION                   39000
#define WIZARD_DURATION_SECONDS           39
#define WIZARD_FINISHED_DURATION          5000
#define WIZARD_SWITCH_SCORE               5000
#define WIZARD_MODE_REWARD_SCORE          250000

#define SPINNER_MAX_GOAL                  100

void ReadStoredParameters() {
  HighScore = BSOS_ReadULFromEEProm(BSOS_HIGHSCORE_EEPROM_START_BYTE, 10000);
  Credits = BSOS_ReadByteFromEEProm(BSOS_CREDITS_EEPROM_BYTE);
  if (Credits > MaximumCredits) Credits = MaximumCredits;

  ReadSetting(EEPROM_FREE_PLAY_BYTE, 0);
  FreePlayMode = (EEPROM.read(EEPROM_FREE_PLAY_BYTE)) ? true : false;

  BallSaveNumSeconds = ReadSetting(EEPROM_BALL_SAVE_BYTE, 15);
  if (BallSaveNumSeconds > 20) BallSaveNumSeconds = 20;

  MusicLevel = ReadSetting(EEPROM_MUSIC_LEVEL_BYTE, 3);
  if (MusicLevel > 3) MusicLevel = 3;

  TournamentScoring = (ReadSetting(EEPROM_TOURNAMENT_SCORING_BYTE, 0)) ? true : false;

  MaxTiltWarnings = ReadSetting(EEPROM_TILT_WARNING_BYTE, 2);
  if (MaxTiltWarnings > 2) MaxTiltWarnings = 2;

  byte awardOverride = ReadSetting(EEPROM_AWARD_OVERRIDE_BYTE, 99);
  if (awardOverride != 99) {
    ScoreAwardReplay = awardOverride;
  }

  byte ballsOverride = ReadSetting(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  if (ballsOverride == 3 || ballsOverride == 5) {
    BallsPerGame = ballsOverride;
  } else {
    if (ballsOverride != 99) EEPROM.write(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  }

  ScrollingScores = (ReadSetting(EEPROM_SCROLLING_SCORES_BYTE, 1)) ? true : false;

  ExtraBallValue = BSOS_ReadULFromEEProm(EEPROM_EXTRA_BALL_SCORE_BYTE);
  if (ExtraBallValue % 1000 || ExtraBallValue > 100000) ExtraBallValue = 20000;

  SpecialValue = BSOS_ReadULFromEEProm(EEPROM_SPECIAL_SCORE_BYTE);
  if (SpecialValue % 1000 || SpecialValue > 100000) SpecialValue = 40000;

  DimLevel = ReadSetting(EEPROM_DIM_LEVEL_BYTE, 2);
  if (DimLevel < 2 || DimLevel > 3) DimLevel = 2;
  BSOS_SetDimDivisor(1, DimLevel);

  AwardScores[0] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_1_EEPROM_START_BYTE);
  AwardScores[1] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_2_EEPROM_START_BYTE);
  AwardScores[2] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_3_EEPROM_START_BYTE);

}


void setup() {
  if (DEBUG_MESSAGES) {
    Serial.begin(57600);
  }

  // Tell the OS about game-specific lights and switches
  BSOS_SetupGameSwitches(NUM_SWITCHES_WITH_TRIGGERS, NUM_PRIORITY_SWITCHES_WITH_TRIGGERS, SolenoidAssociatedSwitches);

  // Set up the chips and interrupts
  BSOS_InitializeMPU();
  BSOS_DisableSolenoidStack();
  BSOS_SetDisableFlippers(true);

  // Read parameters from EEProm
  ReadStoredParameters();
  BSOS_SetCoinLockout((Credits >= MaximumCredits) ? true : false);

  CurrentScores[0] = PINBALL_MACHINE_BASE_MAJOR_VERSION;
  CurrentScores[1] = PINBALL_MACHINE_BASE_MINOR_VERSION;
  CurrentScores[2] = BALLY_STERN_OS_MAJOR_VERSION;
  CurrentScores[3] = BALLY_STERN_OS_MINOR_VERSION;

#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
  // WAV Trigger startup at 57600
  wTrig.start();
  wTrig.stopAllTracks();
  delayMicroseconds(10000);
#endif

#if defined(BALLY_STERN_OS_USE_DASH51)
  ClearSoundQueue();
#endif 

  StopAudio();
  CurrentTime = millis();
  PlaySoundEffect(SOUND_EFFECT_MACHINE_START);

  if (BSOS_ReadSingleSwitchState(SW_KICKER_ROLLOVER)) {
    BSOS_PushToTimedSolenoidStack(SOL_KICKBACK_ARM, 6, CurrentTime + 100, true);
  }
  
}

byte ReadSetting(byte setting, byte defaultValue) {
  byte value = EEPROM.read(setting);
  if (value == 0xFF) {
    EEPROM.write(setting, defaultValue);
    return defaultValue;
  }
  return value;
}


// This function is useful for checking the status of drop target switches
byte CheckSequentialSwitches(byte startingSwitch, byte numSwitches) {
  byte returnSwitches = 0;
  for (byte count = 0; count < numSwitches; count++) {
    returnSwitches |= (BSOS_ReadSingleSwitchState(startingSwitch + count) << count);
  }
  return returnSwitches;
}


byte GetCurrentSilverballWord() {
  byte currentWord = 3;
  for (byte count=0; count<15; count++) {
    if ((SilverballStatus[CurrentPlayer][count]&0x0F)<SilverballMode[CurrentPlayer]) {
      if (count<6) {
        currentWord = 0;
        break;
      } 
      if (count<10) {
        currentWord = 1;
        break;
      }
      currentWord = 2;
      break;
    }
  }
  return currentWord;
}


////////////////////////////////////////////////////////////////////////////
//
//  Lamp Management functions
//
////////////////////////////////////////////////////////////////////////////
byte HeadSilverballLampIndex[10] = {
  52, 53, 54, 55, 56, 57, 58, 59, 44, 45
};


void ShowLampAnimation(byte animationNum, unsigned long divisor, unsigned long baseTime, byte subOffset, boolean dim, boolean reverse = false, byte keepLampOn = 99) {
  byte currentStep = (baseTime / divisor) % LAMP_ANIMATION_STEPS;
  if (reverse) currentStep = (LAMP_ANIMATION_STEPS - 1) - currentStep;

  byte lampNum = 0;
  for (int byteNum = 0; byteNum < 8; byteNum++) {
    for (byte bitNum = 0; bitNum < 8; bitNum++) {

      // if there's a subOffset, turn off lights at that offset
      if (subOffset) {
        byte lampOff = true;
        lampOff = LampAnimations[animationNum][(currentStep + subOffset) % LAMP_ANIMATION_STEPS][byteNum] & (1 << bitNum);
        if (lampOff && lampNum != keepLampOn) BSOS_SetLampState(lampNum, 0);
      }

      byte lampOn = false;
      lampOn = LampAnimations[animationNum][currentStep][byteNum] & (1 << bitNum);
      if (lampOn) BSOS_SetLampState(lampNum, 1, dim);

      lampNum += 1;
    }
#if not defined (BALLY_STERN_OS_SOFTWARE_DISPLAY_INTERRUPT)
    if (byteNum % 2) BSOS_DataRead(0);
#endif
  }
}


void ShowBonusLamps() {
  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT) {
    BSOS_SetLampState(LAMP_15K_BONUS, 0);
    BSOS_SetLampState(LAMP_30K_BONUS, 0);
    BSOS_SetLampState(LAMP_KICKER_SPECIAL, 0);
  } else {
    byte lampPhase = (CurrentTime/200)%21;
    if (lampPhase<9) {
      BSOS_SetLampState(LAMP_15K_BONUS, (AddedBonusQualified[CurrentPlayer])||(AddedBonusAchieved[CurrentPlayer]), 0, AddedBonusQualified[CurrentPlayer]==15?200:0);
      BSOS_SetLampState(LAMP_30K_BONUS, (AddedBonusQualified[CurrentPlayer]==30)||(AddedBonusAchieved[CurrentPlayer]>15), 0, AddedBonusQualified[CurrentPlayer]==30?200:0);
      BSOS_SetLampState(LAMP_KICKER_SPECIAL, (AddedBonusQualified[CurrentPlayer]==60)||(AddedBonusAchieved[CurrentPlayer]>30), 0, AddedBonusQualified[CurrentPlayer]==60?200:0);
    } else {
      if (AddedBonusQualified[CurrentPlayer]) {
        BSOS_SetLampState(LAMP_15K_BONUS, (lampPhase%3)==0, 1);
        BSOS_SetLampState(LAMP_30K_BONUS, (lampPhase%3)==1, 1);
        BSOS_SetLampState(LAMP_KICKER_SPECIAL, (lampPhase%3)==2, 1);
      } else {
        BSOS_SetLampState(LAMP_15K_BONUS, (AddedBonusQualified[CurrentPlayer])||(AddedBonusAchieved[CurrentPlayer]), 0, AddedBonusQualified[CurrentPlayer]==15?200:0);
        BSOS_SetLampState(LAMP_30K_BONUS, (AddedBonusQualified[CurrentPlayer]==30)||(AddedBonusAchieved[CurrentPlayer]>15), 0, AddedBonusQualified[CurrentPlayer]==30?200:0);
        BSOS_SetLampState(LAMP_KICKER_SPECIAL, (AddedBonusQualified[CurrentPlayer]==60)||(AddedBonusAchieved[CurrentPlayer]>30), 0, AddedBonusQualified[CurrentPlayer]==60?200:0);
      }
    }
  }
}


void ShowSpinnerLamps() {
  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT) {
    BSOS_SetLampState(LAMP_LEFT_SPINNER_1000_WHEN_LIT, 0);
    BSOS_SetLampState(LAMP_RIGHT_SPINNER_1000_WHEN_LIT, 0);
  } else {
    boolean leftAlternatingCombo = (AlternatingComboPhase<10 && AlternatingComboPhase%2);
    boolean rightAlternatingCombo = (AlternatingComboPhase && AlternatingComboPhase<10 && ((AlternatingComboPhase%2)==0));
    BSOS_SetLampState(LAMP_LEFT_SPINNER_1000_WHEN_LIT, (Spinner1kPhase==1)||leftAlternatingCombo, 0, leftAlternatingCombo?100:0);
    BSOS_SetLampState(LAMP_RIGHT_SPINNER_1000_WHEN_LIT, (Spinner1kPhase==2)||rightAlternatingCombo, 0, rightAlternatingCombo?100:0);
  }
}


unsigned long NextHorsehoeTime() {
  return 5000 - ((unsigned long)BonusX[CurrentPlayer]*1000);
}

void ShowBonusXLamps() {
  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT) {
    byte lampPhase = ((CurrentTime-GameModeStartTime)/200)%3;
    BSOS_SetLampState(LAMP_BONUS_2X, lampPhase==0);
    BSOS_SetLampState(LAMP_BONUS_3X, lampPhase==0);
    BSOS_SetLampState(LAMP_BONUS_4X, lampPhase==0);
    BSOS_SetLampState(LAMP_BONUS_5X, lampPhase==0);
    BSOS_SetLampState(LAMP_EXTRA_BALL, lampPhase==1);
  } else {
    //    BSOS_SetLampState(LAMP_BONUS_2X, BonusX[CurrentPlayer]==2);
    //    BSOS_SetLampState(LAMP_BONUS_3X, BonusX[CurrentPlayer]==2);
    //    BSOS_SetLampState(LAMP_BONUS_5X, BonusX[CurrentPlayer]==2);    
    if (LastHorseshoe && (CurrentTime-LastHorseshoe)<NextHorsehoeTime()) {
      BSOS_SetLampState(LAMP_BONUS_2X, BonusX[CurrentPlayer]==1, 0, 125);
      BSOS_SetLampState(LAMP_BONUS_3X, BonusX[CurrentPlayer]==2, 0, 125);
      BSOS_SetLampState(LAMP_BONUS_4X, BonusX[CurrentPlayer]==3, 0, 125);
      BSOS_SetLampState(LAMP_BONUS_5X, BonusX[CurrentPlayer]==4, 0, 125);
      BSOS_SetLampState(LAMP_EXTRA_BALL, ((CurrentTime-LastHorseshoe)/125)%2);
    } else {
      BSOS_SetLampState(LAMP_BONUS_2X, BonusX[CurrentPlayer]==2);
      BSOS_SetLampState(LAMP_BONUS_3X, BonusX[CurrentPlayer]==3);
      BSOS_SetLampState(LAMP_BONUS_4X, BonusX[CurrentPlayer]==4);
      BSOS_SetLampState(LAMP_BONUS_5X, BonusX[CurrentPlayer]==5);
      BSOS_SetLampState(LAMP_EXTRA_BALL, 0);
    }
  }
}


void ShowLaneAndRolloverLamps() {

  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT) {
    BSOS_SetLampState(LAMP_TOPLANE_CENTER_WHEN_LIT, ToplanePhase==0, 0, 150);
    BSOS_SetLampState(LAMP_TOPLANE_OUTER_WHEN_LIT, ToplanePhase, 0, 150);
  } else {
    if (ToplaneAnimationEnd) {
      if (CurrentTime>ToplaneAnimationEnd) ToplaneAnimationEnd = 0;
      BSOS_SetLampState(LAMP_TOPLANE_CENTER_WHEN_LIT, 1, 0, 150);
      BSOS_SetLampState(LAMP_TOPLANE_OUTER_WHEN_LIT, 1, 0, 150);
    } else {
      BSOS_SetLampState(LAMP_TOPLANE_CENTER_WHEN_LIT, ToplaneProgress & 0x02);
      if ((ToplaneProgress&0x05)==0x05) {
        BSOS_SetLampState(LAMP_TOPLANE_OUTER_WHEN_LIT, 1);
      } else {
        boolean lampOn = false;
        byte lampPhase = ((CurrentTime/500)%6);
        if (ToplaneProgress&0x01) lampOn = (lampPhase<2);
        else if (ToplaneProgress&0x04) lampOn = (lampPhase==0 || lampPhase==2);
        BSOS_SetLampState(LAMP_TOPLANE_OUTER_WHEN_LIT, lampOn);
      }
    }
  }
}


void ShowKickerLamps() {

  int flash = 500;
  if (KickerTimeout && ((KickerTimeout-CurrentTime)<5000)) flash = 125;
  if (KickerRolloverWatch!=0) flash = 50;
  BSOS_SetLampState(LAMP_KICKER_LAMPS, KickerStatus[CurrentPlayer], 0, flash);
}

void ShowShootAgainLamps() {

  if (!BallSaveUsed && BallSaveNumSeconds > 0 && (CurrentTime - BallFirstSwitchHitTime) < ((unsigned long)(BallSaveNumSeconds - 1) * 1000)) {
    unsigned long msRemaining = ((unsigned long)(BallSaveNumSeconds - 1) * 1000) - (CurrentTime - BallFirstSwitchHitTime);
    BSOS_SetLampState(LAMP_SHOOT_AGAIN, 1, 0, (msRemaining < 1000) ? 100 : 500);
  } else {
    BSOS_SetLampState(LAMP_SHOOT_AGAIN, SamePlayerShootsAgain);
  }
}


void ShowSilverballLamps() {
  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT) {
//    SilverballPhase = (((CurrentTime-GameModeStartTime)/250)%15) + 1;
//    for (int count=0; count<15; count++) {
//      BSOS_SetLampState(LAMP_PLAYFIELD_STANDUP_S+count, (count==(SilverballPhase-1)));
//    }
    byte lampPhase = ((CurrentTime-GameModeStartTime)/200)%3;

    for (int count=0; count<15; count++) {
      if((count+LAMP_PLAYFIELD_STANDUP_S)!=LAMP_PLAYFIELD_STANDUP_N) BSOS_SetLampState(LAMP_PLAYFIELD_STANDUP_S+count, 0);
      else BSOS_SetLampState(LAMP_PLAYFIELD_STANDUP_N, lampPhase==2);
    }       
    
  } else {
    boolean nAlreadyLit = false;
    if (LastHorseshoe && (CurrentTime-LastHorseshoe)<NextHorsehoeTime()) {
      BSOS_SetLampState(LAMP_PLAYFIELD_STANDUP_N, (((CurrentTime-LastHorseshoe)/125)%2)==0);
      nAlreadyLit = true;
    }
    if (SilverballMode[CurrentPlayer]==SILVERBALL_MODE_KNOCK_OUT_LIGHTS) {
      for (int count=0; count<15; count++) {
        if (nAlreadyLit && (LAMP_PLAYFIELD_STANDUP_S+count)==LAMP_PLAYFIELD_STANDUP_N) continue;
        BSOS_SetLampState(LAMP_PLAYFIELD_STANDUP_S+count, (SilverballStatus[CurrentPlayer][count]&0x0F)<SILVERBALL_MODE_KNOCK_OUT_LIGHTS);
      }
    } else if (SilverballMode[CurrentPlayer]==SILVERBALL_MODE_WORD_GROUPS) {      
      for (int count=0; count<15; count++) {
        if (count<6) BSOS_SetLampState(LAMP_PLAYFIELD_STANDUP_S+count, (SilverballStatus[CurrentPlayer][count]&0x0F)<SILVERBALL_MODE_WORD_GROUPS && CurrentSilverballWord==0);
        else if (count<10) BSOS_SetLampState(LAMP_PLAYFIELD_STANDUP_S+count, (SilverballStatus[CurrentPlayer][count]&0x0F)<SILVERBALL_MODE_WORD_GROUPS && CurrentSilverballWord==1);
        else BSOS_SetLampState(LAMP_PLAYFIELD_STANDUP_S+count, (SilverballStatus[CurrentPlayer][count]&0x0F)<SILVERBALL_MODE_WORD_GROUPS && CurrentSilverballWord==2);
      }
    } else if (SilverballMode[CurrentPlayer]>=SILVERBALL_MODE_FADEAWAY_LETTERS) {
      for (int count=0; count<15; count++) {
        if (nAlreadyLit && (LAMP_PLAYFIELD_STANDUP_S+count)==LAMP_PLAYFIELD_STANDUP_N) continue;
        int lampFlash = 0;
        if (SilverballHighlightEnd[count]) lampFlash = 300;
        BSOS_SetLampState(LAMP_PLAYFIELD_STANDUP_S+count, (SilverballStatus[CurrentPlayer][count]&0x0F)<SILVERBALL_MODE_FADEAWAY_LETTERS, 0, lampFlash);
      }      
    }
  }

  for (int count=0; count<15; count++) {
    int flashTime = 100;
    if (SilverballHighlightEnd[count] && (SilverballHighlightEnd[count]-CurrentTime)>5000) flashTime = 300;
    if (CurrentTime<SilverballHighlightEnd[count]) BSOS_SetLampState(LAMP_PLAYFIELD_SPELLOUT_S+count, 1, 0, flashTime);
    else BSOS_SetLampState(LAMP_PLAYFIELD_SPELLOUT_S+count, (SilverballStatus[CurrentPlayer][count]&0x0F)==SilverballMode[CurrentPlayer]);
  }

  for (int count=0; count<10; count++) {
    BSOS_SetLampState(HeadSilverballLampIndex[count], SilverballStatus[CurrentPlayer][count]>0x0F, 0, 1000/(SilverballStatus[CurrentPlayer][count]/16));
  }
  
}



////////////////////////////////////////////////////////////////////////////
//
//  Display Management functions
//
////////////////////////////////////////////////////////////////////////////
unsigned long LastTimeScoreChanged = 0;
unsigned long LastTimeOverrideAnimated = 0;
unsigned long LastFlashOrDash = 0;
#ifdef USE_SCORE_OVERRIDES
unsigned long ScoreOverrideValue[4] = {0, 0, 0, 0};
byte ScoreOverrideStatus = 0;
#define DISPLAY_OVERRIDE_BLANK_SCORE 0xFFFFFFFF
#endif
byte LastScrollPhase = 0;

byte MagnitudeOfScore(unsigned long score) {
  if (score == 0) return 0;

  byte retval = 0;
  while (score > 0) {
    score = score / 10;
    retval += 1;
  }
  return retval;
}

#ifdef USE_SCORE_OVERRIDES
void OverrideScoreDisplay(byte displayNum, unsigned long value, boolean animate) {
  if (displayNum > 3) return;
  ScoreOverrideStatus |= (0x10 << displayNum);
  if (animate) ScoreOverrideStatus |= (0x01 << displayNum);
  else ScoreOverrideStatus &= ~(0x01 << displayNum);
  ScoreOverrideValue[displayNum] = value;
}
#endif

byte GetDisplayMask(byte numDigits) {
  byte displayMask = 0;
  for (byte digitCount = 0; digitCount < numDigits; digitCount++) {
    displayMask |= (0x20 >> digitCount);
  }
  return displayMask;
}


void ShowPlayerScores(byte displayToUpdate, boolean flashCurrent, boolean dashCurrent, unsigned long allScoresShowValue = 0) {

#ifdef USE_SCORE_OVERRIDES
  if (displayToUpdate == 0xFF) ScoreOverrideStatus = 0;
#endif

  byte displayMask = 0x3F;
  unsigned long displayScore = 0;
  unsigned long overrideAnimationSeed = CurrentTime / 250;
  byte scrollPhaseChanged = false;

  byte scrollPhase = ((CurrentTime - LastTimeScoreChanged) / 250) % 16;
  if (scrollPhase != LastScrollPhase) {
    LastScrollPhase = scrollPhase;
    scrollPhaseChanged = true;
  }

  boolean updateLastTimeAnimated = false;

  for (byte scoreCount = 0; scoreCount < 4; scoreCount++) {

#ifdef USE_SCORE_OVERRIDES
    // If this display is currently being overriden, then we should update it
    if (allScoresShowValue == 0 && (ScoreOverrideStatus & (0x10 << scoreCount))) {
      displayScore = ScoreOverrideValue[scoreCount];
      if (displayScore != DISPLAY_OVERRIDE_BLANK_SCORE) {
        byte numDigits = MagnitudeOfScore(displayScore);
        if (numDigits == 0) numDigits = 1;
        if (numDigits < (BALLY_STERN_OS_NUM_DIGITS - 1) && (ScoreOverrideStatus & (0x01 << scoreCount))) {
          // This score is going to be animated (back and forth)
          if (overrideAnimationSeed != LastTimeOverrideAnimated) {
            updateLastTimeAnimated = true;
            byte shiftDigits = (overrideAnimationSeed) % (((BALLY_STERN_OS_NUM_DIGITS + 1) - numDigits) + ((BALLY_STERN_OS_NUM_DIGITS - 1) - numDigits));
            if (shiftDigits >= ((BALLY_STERN_OS_NUM_DIGITS + 1) - numDigits)) shiftDigits = (BALLY_STERN_OS_NUM_DIGITS - numDigits) * 2 - shiftDigits;
            byte digitCount;
            displayMask = GetDisplayMask(numDigits);
            for (digitCount = 0; digitCount < shiftDigits; digitCount++) {
              displayScore *= 10;
              displayMask = displayMask >> 1;
            }
            BSOS_SetDisplayBlank(scoreCount, 0x00);
            BSOS_SetDisplay(scoreCount, displayScore, false);
            BSOS_SetDisplayBlank(scoreCount, displayMask);
          }
        } else {
          BSOS_SetDisplay(scoreCount, displayScore, true, 1);
        }
      } else {
        BSOS_SetDisplayBlank(scoreCount, 0);
      }

    } else {
#endif
      // No override, update scores designated by displayToUpdate
      //CurrentScores[CurrentPlayer] = CurrentScoreOfCurrentPlayer;
      if (allScoresShowValue == 0) displayScore = CurrentScores[scoreCount];
      else displayScore = allScoresShowValue;

      // If we're updating all displays, or the one currently matching the loop, or if we have to scroll
      if (displayToUpdate == 0xFF || displayToUpdate == scoreCount || displayScore > BALLY_STERN_OS_MAX_DISPLAY_SCORE) {

        // Don't show this score if it's not a current player score (even if it's scrollable)
        if (displayToUpdate == 0xFF && (scoreCount >= CurrentNumPlayers && CurrentNumPlayers != 0) && allScoresShowValue == 0) {
          BSOS_SetDisplayBlank(scoreCount, 0x00);
          continue;
        }

        if (displayScore > BALLY_STERN_OS_MAX_DISPLAY_SCORE) {
          // Score needs to be scrolled
          if ((CurrentTime - LastTimeScoreChanged) < 4000) {
            BSOS_SetDisplay(scoreCount, displayScore % (BALLY_STERN_OS_MAX_DISPLAY_SCORE + 1), false);
            BSOS_SetDisplayBlank(scoreCount, BALLY_STERN_OS_ALL_DIGITS_MASK);
          } else {

            // Scores are scrolled 10 digits and then we wait for 6
            if (scrollPhase < 11 && scrollPhaseChanged) {
              byte numDigits = MagnitudeOfScore(displayScore);

              // Figure out top part of score
              unsigned long tempScore = displayScore;
              if (scrollPhase < BALLY_STERN_OS_NUM_DIGITS) {
                displayMask = BALLY_STERN_OS_ALL_DIGITS_MASK;
                for (byte scrollCount = 0; scrollCount < scrollPhase; scrollCount++) {
                  displayScore = (displayScore % (BALLY_STERN_OS_MAX_DISPLAY_SCORE + 1)) * 10;
                  displayMask = displayMask >> 1;
                }
              } else {
                displayScore = 0;
                displayMask = 0x00;
              }

              // Add in lower part of score
              if ((numDigits + scrollPhase) > 10) {
                byte numDigitsNeeded = (numDigits + scrollPhase) - 10;
                for (byte scrollCount = 0; scrollCount < (numDigits - numDigitsNeeded); scrollCount++) {
                  tempScore /= 10;
                }
                displayMask |= GetDisplayMask(MagnitudeOfScore(tempScore));
                displayScore += tempScore;
              }
              BSOS_SetDisplayBlank(scoreCount, displayMask);
              BSOS_SetDisplay(scoreCount, displayScore);
            }
          }
        } else {
          if (flashCurrent) {
            unsigned long flashSeed = CurrentTime / 250;
            if (flashSeed != LastFlashOrDash) {
              LastFlashOrDash = flashSeed;
              if (((CurrentTime / 250) % 2) == 0) BSOS_SetDisplayBlank(scoreCount, 0x00);
              else BSOS_SetDisplay(scoreCount, displayScore, true, 2);
            }
          } else if (dashCurrent) {
            unsigned long dashSeed = CurrentTime / 50;
            if (dashSeed != LastFlashOrDash) {
              LastFlashOrDash = dashSeed;
              byte dashPhase = (CurrentTime / 60) % 36;
              byte numDigits = MagnitudeOfScore(displayScore);
              if (dashPhase < 12) {
                displayMask = GetDisplayMask((numDigits == 0) ? 2 : numDigits);
                if (dashPhase < 7) {
                  for (byte maskCount = 0; maskCount < dashPhase; maskCount++) {
                    displayMask &= ~(0x01 << maskCount);
                  }
                } else {
                  for (byte maskCount = 12; maskCount > dashPhase; maskCount--) {
                    displayMask &= ~(0x20 >> (maskCount - dashPhase - 1));
                  }
                }
                BSOS_SetDisplay(scoreCount, displayScore);
                BSOS_SetDisplayBlank(scoreCount, displayMask);
              } else {
                BSOS_SetDisplay(scoreCount, displayScore, true, 2);
              }
            }
          } else {
            BSOS_SetDisplay(scoreCount, displayScore, true, 2);
          }
        }
      } // End if this display should be updated
#ifdef USE_SCORE_OVERRIDES
    } // End on non-overridden
#endif
  } // End loop on scores

  if (updateLastTimeAnimated) {
    LastTimeOverrideAnimated = overrideAnimationSeed;
  }

}

void ShowFlybyValue(byte numToShow, unsigned long timeBase) {
  byte shiftDigits = (CurrentTime - timeBase) / 120;
  byte rightSideBlank = 0;

  unsigned long bigVersionOfNum = (unsigned long)numToShow;
  for (byte count = 0; count < shiftDigits; count++) {
    bigVersionOfNum *= 10;
    rightSideBlank /= 2;
    if (count > 2) rightSideBlank |= 0x20;
  }
  bigVersionOfNum /= 1000;

  byte curMask = BSOS_SetDisplay(CurrentPlayer, bigVersionOfNum, false, 0);
  if (bigVersionOfNum == 0) curMask = 0;
  BSOS_SetDisplayBlank(CurrentPlayer, ~(~curMask | rightSideBlank));
}

/*

  XXdddddd---
           10
          100
         1000
        10000
       10x000
      10xx000
     10xxx000
    10xxxx000
   10xxxxx000
  10xxxxxx000
*/

////////////////////////////////////////////////////////////////////////////
//
//  Machine State Helper functions
//
////////////////////////////////////////////////////////////////////////////
boolean AddPlayer(boolean resetNumPlayers = false) {

  if (Credits < 1 && !FreePlayMode) return false;
  if (resetNumPlayers) CurrentNumPlayers = 0;
  if (CurrentNumPlayers >= 4) return false;

  CurrentNumPlayers += 1;
  BSOS_SetDisplay(CurrentNumPlayers - 1, 0);
  BSOS_SetDisplayBlank(CurrentNumPlayers - 1, 0x30);

  if (!FreePlayMode) {
    Credits -= 1;
    BSOS_WriteByteToEEProm(BSOS_CREDITS_EEPROM_BYTE, Credits);
    BSOS_SetDisplayCredits(Credits);
    BSOS_SetCoinLockout(false);
  }
  PlaySoundEffect(SOUND_EFFECT_ADD_PLAYER_1 + (CurrentNumPlayers - 1));

  BSOS_WriteULToEEProm(BSOS_TOTAL_PLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_PLAYS_EEPROM_START_BYTE) + 1);

  return true;
}

void AddCoinToAudit(byte switchHit) {

  unsigned short coinAuditStartByte = 0;

  switch (switchHit) {
    case SW_COIN_3: coinAuditStartByte = BSOS_CHUTE_3_COINS_START_BYTE; break;
    case SW_COIN_2: coinAuditStartByte = BSOS_CHUTE_2_COINS_START_BYTE; break;
    case SW_COIN_1: coinAuditStartByte = BSOS_CHUTE_1_COINS_START_BYTE; break;
  }

  if (coinAuditStartByte) {
    BSOS_WriteULToEEProm(coinAuditStartByte, BSOS_ReadULFromEEProm(coinAuditStartByte) + 1);
  }

}


void AddCredit(boolean playSound = false, byte numToAdd = 1) {
  if (Credits < MaximumCredits) {
    Credits += numToAdd;
    if (Credits > MaximumCredits) Credits = MaximumCredits;
    BSOS_WriteByteToEEProm(BSOS_CREDITS_EEPROM_BYTE, Credits);
    if (playSound) PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT);
    BSOS_SetDisplayCredits(Credits);
    BSOS_SetCoinLockout(false);
  } else {
    BSOS_SetDisplayCredits(Credits);
    BSOS_SetCoinLockout(true);
  }

}

void AddSpecialCredit() {
  AddCredit(false, 1);
  BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
  BSOS_WriteULToEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
}

void AwardSpecial() {
  if (SpecialCollected) return;
  SpecialCollected = true;
  if (TournamentScoring) {
    CurrentScores[CurrentPlayer] += SpecialValue * ScoreMultiplier;
  } else {
    AddSpecialCredit();
  }
}

void AwardExtraBall() {
  if (ExtraBallCollected) return;
  ExtraBallCollected = true;
  if (TournamentScoring) {
    CurrentScores[CurrentPlayer] += ExtraBallValue * ScoreMultiplier;
  } else {
    SamePlayerShootsAgain = true;
    BSOS_SetLampState(LAMP_SHOOT_AGAIN, SamePlayerShootsAgain);
    StopAudio();
    PlaySoundEffect(SOUND_EFFECT_EXTRA_BALL);
    //    PlayBackgroundSongBasedOnLevel(StarLevel[CurrentPlayer]);
    //ResumeBackgroundSong();
  }
}

#define ADJ_TYPE_LIST                 1
#define ADJ_TYPE_MIN_MAX              2
#define ADJ_TYPE_MIN_MAX_DEFAULT      3
#define ADJ_TYPE_SCORE                4
#define ADJ_TYPE_SCORE_WITH_DEFAULT   5
#define ADJ_TYPE_SCORE_NO_DEFAULT     6
byte AdjustmentType = 0;
byte NumAdjustmentValues = 0;
byte AdjustmentValues[8];
unsigned long AdjustmentScore;
byte *CurrentAdjustmentByte = NULL;
unsigned long *CurrentAdjustmentUL = NULL;
byte CurrentAdjustmentStorageByte = 0;
byte TempValue = 0;


int RunSelfTest(int curState, boolean curStateChanged) {
  int returnState = curState;
  CurrentNumPlayers = 0;

  if (curStateChanged) {
    // Send a stop-all command and reset the sample-rate offset, in case we have
    //  reset while the WAV Trigger was already playing.
    StopAudio();
  }

  // Any state that's greater than CHUTE_3 is handled by the Base Self-test code
  // Any that's less, is machine specific, so we handle it here.
  if (curState >= MACHINE_STATE_TEST_CHUTE_3_COINS) {
    returnState = RunBaseSelfTest(returnState, curStateChanged, CurrentTime, SW_CREDIT_RESET, SW_SLAM);
  } else {
    byte curSwitch = BSOS_PullFirstFromSwitchStack();

    if (curSwitch == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
      SetLastSelfTestChangedTime(CurrentTime);
      returnState -= 1;
    }

    if (curSwitch == SW_SLAM) {
      returnState = MACHINE_STATE_ATTRACT;
    }

    if (curStateChanged) {
      for (int count = 0; count < 4; count++) {
        BSOS_SetDisplay(count, 0);
        BSOS_SetDisplayBlank(count, 0x00);
      }
      BSOS_SetDisplayCredits(MACHINE_STATE_TEST_SOUNDS - curState);
      BSOS_SetDisplayBallInPlay(0, false);
      CurrentAdjustmentByte = NULL;
      CurrentAdjustmentUL = NULL;
      CurrentAdjustmentStorageByte = 0;

      AdjustmentType = ADJ_TYPE_MIN_MAX;
      AdjustmentValues[0] = 0;
      AdjustmentValues[1] = 1;
      TempValue = 0;

      switch (curState) {
        case MACHINE_STATE_ADJUST_FREEPLAY:
          CurrentAdjustmentByte = (byte *)&FreePlayMode;
          CurrentAdjustmentStorageByte = EEPROM_FREE_PLAY_BYTE;
          break;
        case MACHINE_STATE_ADJUST_BALL_SAVE:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 5;
          AdjustmentValues[1] = 5;
          AdjustmentValues[2] = 10;
          AdjustmentValues[3] = 15;
          AdjustmentValues[4] = 20;
          CurrentAdjustmentByte = &BallSaveNumSeconds;
          CurrentAdjustmentStorageByte = EEPROM_BALL_SAVE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_MUSIC_LEVEL:
          AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
          AdjustmentValues[1] = 3;
          CurrentAdjustmentByte = &MusicLevel;
          CurrentAdjustmentStorageByte = EEPROM_MUSIC_LEVEL_BYTE;
          break;
        case MACHINE_STATE_ADJUST_TOURNAMENT_SCORING:
          CurrentAdjustmentByte = (byte *)&TournamentScoring;
          CurrentAdjustmentStorageByte = EEPROM_TOURNAMENT_SCORING_BYTE;
          break;
        case MACHINE_STATE_ADJUST_TILT_WARNING:
          AdjustmentValues[1] = 2;
          CurrentAdjustmentByte = &MaxTiltWarnings;
          CurrentAdjustmentStorageByte = EEPROM_TILT_WARNING_BYTE;
          break;
        case MACHINE_STATE_ADJUST_AWARD_OVERRIDE:
          AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
          AdjustmentValues[1] = 7;
          CurrentAdjustmentByte = &ScoreAwardReplay;
          CurrentAdjustmentStorageByte = EEPROM_AWARD_OVERRIDE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_BALLS_OVERRIDE:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 3;
          AdjustmentValues[0] = 3;
          AdjustmentValues[1] = 5;
          AdjustmentValues[2] = 99;
          CurrentAdjustmentByte = &BallsPerGame;
          CurrentAdjustmentStorageByte = EEPROM_BALLS_OVERRIDE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SCROLLING_SCORES:
          CurrentAdjustmentByte = (byte *)&ScrollingScores;
          CurrentAdjustmentStorageByte = EEPROM_SCROLLING_SCORES_BYTE;
          break;

        case MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD:
          AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
          CurrentAdjustmentUL = &ExtraBallValue;
          CurrentAdjustmentStorageByte = EEPROM_EXTRA_BALL_SCORE_BYTE;
          break;

        case MACHINE_STATE_ADJUST_SPECIAL_AWARD:
          AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
          CurrentAdjustmentUL = &SpecialValue;
          CurrentAdjustmentStorageByte = EEPROM_SPECIAL_SCORE_BYTE;
          break;

        case MACHINE_STATE_ADJUST_DIM_LEVEL:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 2;
          AdjustmentValues[0] = 2;
          AdjustmentValues[1] = 3;
          CurrentAdjustmentByte = &DimLevel;
          CurrentAdjustmentStorageByte = EEPROM_DIM_LEVEL_BYTE;
          //          for (int count = 0; count < 7; count++) BSOS_SetLampState(MIDDLE_ROCKET_7K + count, 1, 1);
          break;

        case MACHINE_STATE_ADJUST_DONE:
          returnState = MACHINE_STATE_ATTRACT;
          break;
      }

    }

    // Change value, if the switch is hit
    if (curSwitch == SW_CREDIT_RESET) {

      if (CurrentAdjustmentByte && (AdjustmentType == ADJ_TYPE_MIN_MAX || AdjustmentType == ADJ_TYPE_MIN_MAX_DEFAULT)) {
        byte curVal = *CurrentAdjustmentByte;
        curVal += 1;
        if (curVal > AdjustmentValues[1]) {
          if (AdjustmentType == ADJ_TYPE_MIN_MAX) curVal = AdjustmentValues[0];
          else {
            if (curVal > 99) curVal = AdjustmentValues[0];
            else curVal = 99;
          }
        }
        *CurrentAdjustmentByte = curVal;
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, curVal);
      } else if (CurrentAdjustmentByte && AdjustmentType == ADJ_TYPE_LIST) {
        byte valCount = 0;
        byte curVal = *CurrentAdjustmentByte;
        byte newIndex = 0;
        for (valCount = 0; valCount < (NumAdjustmentValues - 1); valCount++) {
          if (curVal == AdjustmentValues[valCount]) newIndex = valCount + 1;
        }
        *CurrentAdjustmentByte = AdjustmentValues[newIndex];
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, AdjustmentValues[newIndex]);
      } else if (CurrentAdjustmentUL && (AdjustmentType == ADJ_TYPE_SCORE_WITH_DEFAULT || AdjustmentType == ADJ_TYPE_SCORE_NO_DEFAULT)) {
        unsigned long curVal = *CurrentAdjustmentUL;
        curVal += 5000;
        if (curVal > 100000) curVal = 0;
        if (AdjustmentType == ADJ_TYPE_SCORE_NO_DEFAULT && curVal == 0) curVal = 5000;
        *CurrentAdjustmentUL = curVal;
        if (CurrentAdjustmentStorageByte) BSOS_WriteULToEEProm(CurrentAdjustmentStorageByte, curVal);
      }

      if (curState == MACHINE_STATE_ADJUST_DIM_LEVEL) {
        BSOS_SetDimDivisor(1, DimLevel);
      }
    }

    // Show current value
    if (CurrentAdjustmentByte != NULL) {
      BSOS_SetDisplay(0, (unsigned long)(*CurrentAdjustmentByte), true);
    } else if (CurrentAdjustmentUL != NULL) {
      BSOS_SetDisplay(0, (*CurrentAdjustmentUL), true);
    }

  }

  if (curState == MACHINE_STATE_ADJUST_DIM_LEVEL) {
    //    for (int count = 0; count < 7; count++) BSOS_SetLampState(MIDDLE_ROCKET_7K + count, 1, (CurrentTime / 1000) % 2);
  }

  if (returnState == MACHINE_STATE_ATTRACT) {
    // If any variables have been set to non-override (99), return
    // them to dip switch settings
    // Balls Per Game, Player Loses On Ties, Novelty Scoring, Award Score
    //    DecodeDIPSwitchParameters();
    BSOS_SetDisplayCredits(Credits, true);
    ReadStoredParameters();
  }

  return returnState;
}




////////////////////////////////////////////////////////////////////////////
//
//  Audio Output functions
//
////////////////////////////////////////////////////////////////////////////

#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
byte CurrentBackgroundSong = SOUND_EFFECT_NONE;
#endif

void StopAudio() {
#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
  wTrig.stopAllTracks();
  CurrentBackgroundSong = SOUND_EFFECT_NONE;
#endif
}

void ResumeBackgroundSong() {
#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
  byte curSong = CurrentBackgroundSong;
  CurrentBackgroundSong = SOUND_EFFECT_NONE;
  PlayBackgroundSong(curSong);
#endif
}

void PlayBackgroundSong(byte songNum) {

#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
  if (MusicLevel > 1) {
    if (CurrentBackgroundSong != songNum) {
      if (CurrentBackgroundSong != SOUND_EFFECT_NONE) wTrig.trackStop(CurrentBackgroundSong);
      if (songNum != SOUND_EFFECT_NONE) {
#ifdef USE_WAV_TRIGGER_1p3
        wTrig.trackPlayPoly(songNum, true);
#else
        wTrig.trackPlayPoly(songNum);
#endif
        wTrig.trackLoop(songNum, true);
        wTrig.trackGain(songNum, -4);
      }
      CurrentBackgroundSong = songNum;
    }
  }
#else
  byte test = songNum;
  songNum = test;
#endif

}


#ifdef BALLY_STERN_OS_USE_DASH51
#define SOUND_QUEUE_SIZE 30
struct SoundQueueEntry {
  byte soundByte;
  unsigned long playTime;
};
SoundQueueEntry SoundQueue[SOUND_QUEUE_SIZE];

void ClearSoundQueue() {
  for (int count=0; count<SOUND_QUEUE_SIZE; count++) {
    SoundQueue[count].soundByte = 0xFF;
  }
}

boolean AddToSoundQueue(byte soundByte, unsigned long playTime) {
  for (int count=0; count<SOUND_QUEUE_SIZE; count++) {
    if (SoundQueue[count].soundByte==0xFF) {
      SoundQueue[count].soundByte = soundByte;
      SoundQueue[count].playTime = playTime;
      return true;
    }
  }
  return false;
}

byte GetFromSoundQueue(unsigned long pullTime) {
  for (int count=0; count<SOUND_QUEUE_SIZE; count++) {
    if (SoundQueue[count].soundByte!=0xFF && SoundQueue[count].playTime<pullTime) {
      byte retSound = SoundQueue[count].soundByte;
      SoundQueue[count].soundByte = 0xFF;
      return retSound;
    }
  }

  return 0xFF;
}

#endif


unsigned long NextSoundEffectTime = 0;

void PlaySoundEffect(byte soundEffectNum) {

  if (MusicLevel == 0) return;

#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)

#ifndef USE_WAV_TRIGGER_1p3
  if (  soundEffectNum == SOUND_EFFECT_THUMPER_BUMPER_HIT ||
        SOUND_EFFECT_SPINNER ) wTrig.trackStop(soundEffectNum);
#endif
  wTrig.trackPlayPoly(soundEffectNum);
  //  char buf[128];
  //  sprintf(buf, "s=%d\n", soundEffectNum);
  //  Serial.write(buf);
#endif

#ifdef BALLY_STERN_OS_USE_DASH51
  switch(soundEffectNum) {
    case SOUND_EFFECT_BONUS_COUNT_LO:
      BSOS_PlaySoundDash51(25);
      break;
    case SOUND_EFFECT_BONUS_COUNT_HI:
      BSOS_PlaySoundDash51(31);
      break;
    case SOUND_EFFECT_BUMPER_HIT:
      BSOS_PlaySoundDash51(13); 
      break;
    case SOUND_EFFECT_SLING_SHOT:
      BSOS_PlaySoundDash51(30); 
      break;
    case SOUND_EFFECT_KICKER_WATCH:
      BSOS_PlaySoundDash51(7); 
      break;
    case SOUND_EFFECT_KICKER_LAUNCH:
      BSOS_PlaySoundDash51(6);
      break;
    case SOUND_EFFECT_ANIMATION_TICK:
      BSOS_PlaySoundDash51(15);
      break;
    case SOUND_EFFECT_50PT_SWITCH:
      AddToSoundQueue(15, CurrentTime);
      AddToSoundQueue(15, CurrentTime+100);
      AddToSoundQueue(15, CurrentTime+200);
      AddToSoundQueue(15, CurrentTime+300);
      AddToSoundQueue(15, CurrentTime+400);
      break;
    case SOUND_EFFECT_BONUS_X_INCREASED:
      AddToSoundQueue(7, CurrentTime);
      AddToSoundQueue(10, CurrentTime+1000);
      break;
    case SOUND_EFFECT_LIGHT_LETTER:
      BSOS_PlaySoundDash51(4); // splash
      break;
    case SOUND_EFFECT_BALL_OVER:
    case SOUND_EFFECT_GAME_OVER:
      BSOS_PlaySoundDash51(5); // falling tone
      break;
    case SOUND_EFFECT_SKILL_SHOT:
    case SOUND_EFFECT_EXTRA_BALL:
    case SOUND_EFFECT_MACHINE_START:
      BSOS_PlaySoundDash51(6); // explosion
      break;
    case SOUND_EFFECT_ADD_CREDIT:
      BSOS_PlaySoundDash51(7); // siren
      break;
    case SOUND_EFFECT_ADD_PLAYER_1:
    case SOUND_EFFECT_ADD_PLAYER_2:
    case SOUND_EFFECT_ADD_PLAYER_3:
    case SOUND_EFFECT_ADD_PLAYER_4:
//      BSOS_PlaySoundDash51(8); // rising sound
//03 02 83 82 0A 0A 28 10 FF
      AddToSoundQueue(3, CurrentTime);
      AddToSoundQueue(2, CurrentTime+200);
      AddToSoundQueue(19, CurrentTime+400);
      AddToSoundQueue(18, CurrentTime+600);
      AddToSoundQueue(10, CurrentTime+800);
      AddToSoundQueue(10, CurrentTime+1000);
      AddToSoundQueue(8, CurrentTime+1200);
//      AddToSoundQueue(0, CurrentTime+1400);
      break;
    case SOUND_EFFECT_ROLLOVER:
      BSOS_PlaySoundDash51(25);
      break;
    case SOUND_EFFECT_TILT:
      BSOS_PlaySoundDash51(14); // warning klaxon
      break;
    case SOUND_EFFECT_TILT_WARNING:
      BSOS_PlaySoundDash51(12); // warning klaxon
      break;
    case SOUND_EFFECT_ADDED_BONUS_QUALIFIED:
      AddToSoundQueue(30, CurrentTime);
      AddToSoundQueue(30, CurrentTime+200);
      AddToSoundQueue(30, CurrentTime+400);
      AddToSoundQueue(31, CurrentTime+600);
      break;
    case SOUND_EFFECT_ADDED_BONUS_COLLECT:
      AddToSoundQueue(29, CurrentTime);
      AddToSoundQueue(25, CurrentTime+200);
      AddToSoundQueue(29, CurrentTime+400);
      AddToSoundQueue(31, CurrentTime+600);
      break;
    case SOUND_EFFECT_HORSESHOE:
      BSOS_PlaySoundDash51(13); // Short explosion
      break;
    case SOUND_EFFECT_SILVERBALL_COMPLETION:
//      BSOS_PlaySoundDash51(6); // falling tone
      AddToSoundQueue(5, CurrentTime+250);
      AddToSoundQueue(6, CurrentTime+1500);
      break;
    case SOUND_EFFECT_SPINNER_HIGH:
      BSOS_PlaySoundDash51(30); 
      break;
    case SOUND_EFFECT_SPINNER_LOW:
    case SOUND_EFFECT_MATCH_SPIN:
      BSOS_PlaySoundDash51(26); 
      break;
  }
#endif  

  if (DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf, "Sound # %d\n", soundEffectNum);
    Serial.write(buf);
  }

}

inline void StopSoundEffect(byte soundEffectNum) {
#if defined(USE_WAV_TRIGGER) || defined(USE_WAV_TRIGGER_1p3)
  wTrig.trackStop(soundEffectNum);
#else
  if (DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf, "Sound # %d\n", soundEffectNum);
    Serial.write(buf);
  }
#endif
}

////////////////////////////////////////////////////////////////////////////
//
//  Attract Mode
//
////////////////////////////////////////////////////////////////////////////

unsigned long AttractLastLadderTime = 0;
byte AttractLastLadderBonus = 0;
unsigned long AttractDisplayRampStart = 0;
byte AttractLastHeadMode = 255;
byte AttractLastPlayfieldMode = 255;
byte InAttractMode = false;


int RunAttractMode(int curState, boolean curStateChanged) {

  int returnState = curState;

  if (curStateChanged) {
    BSOS_DisableSolenoidStack();
    BSOS_TurnOffAllLamps();
    BSOS_SetDisableFlippers(true);
    if (DEBUG_MESSAGES) {
      Serial.write("Entering Attract Mode\n\r");
    }
    AttractLastHeadMode = 0;
    AttractLastPlayfieldMode = 0;
    BSOS_SetDisplayCredits(Credits, true);
  }

  // Alternate displays between high score and blank
  if (CurrentTime < 16000) {
    if (AttractLastHeadMode != 1) {
      ShowPlayerScores(0xFF, false, false);
      BSOS_SetDisplayCredits(Credits, true);
      BSOS_SetDisplayBallInPlay(0, true);
    }
  } else if ((CurrentTime / 8000) % 2 == 0) {

    if (AttractLastHeadMode != 2) {
      BSOS_SetLampState(LAMP_HEAD_HIGH_SCORE, 1, 0, 250);
      BSOS_SetLampState(LAMP_HEAD_GAME_OVER, 0);
      LastTimeScoreChanged = CurrentTime;
    }
    AttractLastHeadMode = 2;
    ShowPlayerScores(0xFF, false, false, HighScore);
  } else {
    if (AttractLastHeadMode != 3) {
      if (CurrentTime < 32000) {
        for (int count = 0; count < 4; count++) {
          CurrentScores[count] = 0;
        }
        CurrentNumPlayers = 0;
      }
      BSOS_SetLampState(LAMP_HEAD_HIGH_SCORE, 0);
      BSOS_SetLampState(LAMP_HEAD_GAME_OVER, 1);
      LastTimeScoreChanged = CurrentTime;
    }
    ShowPlayerScores(0xFF, false, false);

    AttractLastHeadMode = 3;
  }

  byte attractPlayfieldPhase = ((CurrentTime / 5000) % 5);

  if (attractPlayfieldPhase != AttractLastPlayfieldMode) {
    BSOS_TurnOffAllLamps();
    AttractLastPlayfieldMode = attractPlayfieldPhase;
    if (attractPlayfieldPhase == 2) GameMode = GAME_MODE_SKILL_SHOT;
    else GameMode = GAME_MODE_UNSTRUCTURED_PLAY;
    AttractLastLadderBonus = 1;
    AttractLastLadderTime = CurrentTime;
  }

  if (attractPlayfieldPhase < 2) {
    ShowLampAnimation(1, 40, CurrentTime, 14, false, false);
  } else if (attractPlayfieldPhase == 3) {
    ShowLampAnimation(0, 40, CurrentTime, 11, false, false, 46);
  } else if (attractPlayfieldPhase == 2) {
    if ((CurrentTime - AttractLastLadderTime) > 200) {
      CurrentBonus = AttractLastLadderBonus;
      ShowBonusLamps();
      AttractLastLadderBonus += 1;
      if (AttractLastLadderBonus > 20) AttractLastLadderBonus = 0;
      AttractLastLadderTime = CurrentTime;
    }
#if not defined (BALLY_STERN_OS_SOFTWARE_DISPLAY_INTERRUPT)
    BSOS_DataRead(0);
#endif
  } else {
    ShowLampAnimation(2, 40, CurrentTime, 14, false, false);
  }

  byte switchHit;
  while ( (switchHit = BSOS_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
    if (switchHit == SW_CREDIT_RESET) {
      if (AddPlayer(true)) returnState = MACHINE_STATE_INIT_GAMEPLAY;
    }
    if (switchHit == SW_COIN_1 || switchHit == SW_COIN_2 || switchHit == SW_COIN_3) {
      AddCoinToAudit(switchHit);
      AddCredit(true, 1);
    }
    if (switchHit == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
      returnState = MACHINE_STATE_TEST_LIGHTS;
      SetLastSelfTestChangedTime(CurrentTime);
    }
  }

  return returnState;
}





////////////////////////////////////////////////////////////////////////////
//
//  Game Play functions
//
////////////////////////////////////////////////////////////////////////////
byte CountBits(byte byteToBeCounted) {
  byte numBits = 0;

  for (byte count = 0; count < 8; count++) {
    numBits += (byteToBeCounted & 0x01);
    byteToBeCounted = byteToBeCounted >> 1;
  }

  return numBits;
}


void AddToBonus(byte amountToAdd = 1) {
  CurrentBonus += amountToAdd;
  if (CurrentBonus >= MAX_DISPLAY_BONUS) {
    CurrentBonus = MAX_DISPLAY_BONUS;
  }
}


void SetGameMode(byte newGameMode) {
  GameMode = newGameMode | (GameMode & ~GAME_BASE_MODE);
  GameModeStartTime = 0;
  GameModeEndTime = 0;
  if (DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf, "Game mode set to %d\n", newGameMode);
    Serial.write(buf);
  }
}

void StartScoreAnimation(unsigned long scoreToAnimate) {
  if (ScoreAdditionAnimation != 0) {
    CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
  }
  ScoreAdditionAnimation = scoreToAnimate;
  ScoreAdditionAnimationStartTime = CurrentTime;
  LastRemainingAnimatedScoreShown = 0;
}


boolean CheckIfSilverballComplete() {
  // See if all letters have been hit
  boolean allLettersHit = true;
  for (int count=0; count<15; count++) {
    if ((SilverballStatus[CurrentPlayer][count]&0x0F)<SilverballMode[CurrentPlayer]) {
      allLettersHit = false;
      break;
    }
  }
  if (allLettersHit) {
    if ((SilverballMode[CurrentPlayer]&0x0F)<14) SilverballMode[CurrentPlayer] += 1;
    for (int count=0; count<15; count++) {
      SilverballHighlightEnd[count] = CurrentTime + 5000;
      if (count<10) SilverballStatus[CurrentPlayer][count] = (SilverballStatus[CurrentPlayer][count]&0xF0) | ((SilverballMode[CurrentPlayer]-1)*16);
    }
    StartScoreAnimation(SILVERBALL_COMPLETION_AWARD * ScoreMultiplier * (unsigned long)(SilverballMode[CurrentPlayer]&0x0F));
    PlaySoundEffect(SOUND_EFFECT_SILVERBALL_COMPLETION);
  }

  return allLettersHit;
}


void TurnOnSilverballLetter(byte letterNum) {
  
  if ((SilverballStatus[CurrentPlayer][letterNum]&0x0F)<0x0F) {
    SilverballStatus[CurrentPlayer][letterNum] &= 0xF0;
    SilverballStatus[CurrentPlayer][letterNum] |= SilverballMode[CurrentPlayer];
    SilverballHighlightEnd[letterNum] = CurrentTime + 5000;
    PlaySoundEffect(SOUND_EFFECT_LIGHT_LETTER);
    LastSilverballLetterAchieved = CurrentTime;
  }
}


void SpotSilverballLetter() {
  if (SilverballPhase) {
    
  } else {
    int letterCount;
    for (letterCount=0; letterCount<15; letterCount++) {
      if ((SilverballStatus[CurrentPlayer][letterCount]&0x0F)<SilverballMode[CurrentPlayer]) break;
    }

    if (letterCount<15) TurnOnSilverballLetter(letterCount);
  }

  CheckIfSilverballComplete();
}


void SetKickerStatus(boolean kickerOn, unsigned long duration=2000) {

  if (KickerStatus[CurrentPlayer] != kickerOn) {
    if (kickerOn && KickerStatus[CurrentPlayer]==false) {
      BSOS_PushToTimedSolenoidStack(SOL_KICKBACK_ON, 8, CurrentTime+100);
      if (duration) {
        KickerTimeout = CurrentTime + duration;
      }
    } else {
      BSOS_PushToTimedSolenoidStack(SOL_KICKBACK_ARM, 4, CurrentTime);      
    }
    KickerStatus[CurrentPlayer] = kickerOn;
  }
  
}


byte SwitchToLetterConversion[16] = {
  7, 6, 5, 4, 3, 2, 1, 0, 255, 14, 13, 12, 11, 10, 9, 8
};


//#define SILVERBALL_MODE_KNOCK_OUT_LIGHTS  1
//#define SILVERBALL_MODE_WORD_GROUPS       2
//#define SILVERBALL_MODE_FADEAWAY_LETTERS  3

void HandleSilverballHit(byte switchNum) {

  boolean shotAwarded = false;
  LastSilverballSwitchHit = CurrentTime;

  byte letterNum = SwitchToLetterConversion[switchNum-SW_RIGHT_A_TARGET];
  if (SilverballMode[CurrentPlayer]==SILVERBALL_MODE_KNOCK_OUT_LIGHTS) {
    if ((SilverballStatus[CurrentPlayer][letterNum]&0x0F)<SILVERBALL_MODE_KNOCK_OUT_LIGHTS) {
      TurnOnSilverballLetter(letterNum);
      shotAwarded = true;
      CurrentScores[CurrentPlayer] += 1000 * ScoreMultiplier;
      if (letterNum==10 || letterNum==14) SetKickerStatus(true, 5000);
    }
  } else if (SilverballMode[CurrentPlayer]==SILVERBALL_MODE_WORD_GROUPS) {
    byte switchWordNum = 0;
    if (letterNum>5) switchWordNum = 1;
    if (letterNum>9) switchWordNum = 2;
    byte currentWord = GetCurrentSilverballWord();

    if (switchWordNum==currentWord && (SilverballStatus[CurrentPlayer][letterNum]&0x0F)<SILVERBALL_MODE_WORD_GROUPS) {
      TurnOnSilverballLetter(letterNum);
      shotAwarded = true;
      CurrentScores[CurrentPlayer] += 1000 * ScoreMultiplier;
      if (letterNum==10 || letterNum==14) SetKickerStatus(true, 5000);
    }    
  } else if (SilverballMode[CurrentPlayer]>=SILVERBALL_MODE_FADEAWAY_LETTERS) {
    // See if this letter can be collected, or if it's provisional
    boolean letterIsInOrder = true;
    for (byte count=0; count<letterNum; count++) {
      if ((SilverballStatus[CurrentPlayer][count]&0x0F)<SILVERBALL_MODE_FADEAWAY_LETTERS) {
        letterIsInOrder = false;
        break;
      }
    }
    if (letterIsInOrder) {
      TurnOnSilverballLetter(letterNum);
      CurrentScores[CurrentPlayer] += 1000 * ScoreMultiplier;
      for (byte count=letterNum+1; count<15; count++) {
        if (SilverballHighlightEnd[count]) TurnOnSilverballLetter(count);
        else break;
      }
    } else {
      SilverballHighlightEnd[letterNum] = CurrentTime + 20000;
      CurrentScores[CurrentPlayer] += 1000 * ScoreMultiplier;
    }
//    if (letterNum==10 || letterNum==14) SetKickerStatus(true, 5000);
  }

  if (switchNum==SW_CENTER_TARGET && (CurrentTime-LastHorseshoe)<NextHorsehoeTime()) {
    IncreaseBonusX();
    LastHorseshoe = 0;
    if (SuperSkillshotQualified) StartScoreAnimation(15000 * ScoreMultiplier);
    shotAwarded = true;
  }

  CheckIfSilverballComplete();

  CurrentSilverballWord = GetCurrentSilverballWord();  

  if (!shotAwarded) {
    CurrentScores[CurrentPlayer] += 100 * ScoreMultiplier;
    PlaySoundEffect(SOUND_EFFECT_ROLLOVER);    
  }
}


boolean AdvanceAlternatingCombo(byte switchHit) {
  boolean goalAdvanced = false;
  if (AlternatingComboPhase==0) {
    // inlanes can start an alternating combo
    if (AddedBonusAchieved[CurrentPlayer]<60) {
      if (switchHit==SW_LEFT_INLANE_A) {
        AlternatingComboPhase = 2;
        AlternatingComboExpiration = CurrentTime + 5000;
        goalAdvanced = true;
      } else if (switchHit==SW_RIGHT_INLANE_I) {
        AlternatingComboPhase = 1;
        AlternatingComboExpiration = CurrentTime + 5000;
        goalAdvanced = true;
      }
    }
  } else if (AlternatingComboPhase<10) {
    if (AlternatingComboPhase%2) {
      if (switchHit==SW_LEFT_SPINNER) {
        AlternatingCombosHit += 1;
        AlternatingComboExpiration = CurrentTime + 10000;
        AlternatingComboPhase += 1;
        if (AlternatingComboPhase==10) AlternatingComboPhase = 2;
        goalAdvanced = true;
      } else if (switchHit==SW_RIGHT_SPINNER) {
        if (AlternatingCombosHit) AlternatingComboExpiration = CurrentTime + 10000;
      }
    } else {
      if (switchHit==SW_RIGHT_SPINNER) {
        AlternatingCombosHit += 1;
        AlternatingComboExpiration = CurrentTime + 10000;
        AlternatingComboPhase += 1;
        goalAdvanced = true;
      } else if (switchHit==SW_LEFT_SPINNER) {
        if (AlternatingCombosHit) AlternatingComboExpiration = CurrentTime + 10000;
      }      
    }

    // Baseed on number of combos hit - advance phase
    if (AddedBonusAchieved[CurrentPlayer]==0 && AlternatingCombosHit==2) AlternatingComboPhase = 10;
    else if (AddedBonusAchieved[CurrentPlayer]==15 && AlternatingCombosHit==3) AlternatingComboPhase = 10;
    else if (AddedBonusAchieved[CurrentPlayer]==30 && AlternatingCombosHit==4) AlternatingComboPhase = 10;
    if (AlternatingComboPhase==10) {
      AlternatingComboExpiration = CurrentTime + 15000;
    }
  } else if (AlternatingComboPhase==10) {
    if (switchHit==SW_HOOP_ROLLOVER_BUTTON) {
      // Advance the award
      if (AddedBonusAchieved[CurrentPlayer]==0) AddedBonusQualified[CurrentPlayer] = 15;
      else if (AddedBonusAchieved[CurrentPlayer]==15) AddedBonusQualified[CurrentPlayer] = 30;
      else if (AddedBonusAchieved[CurrentPlayer]==30) AddedBonusQualified[CurrentPlayer] = 60;
      AlternatingCombosHit = 0;
      AlternatingComboPhase = 0;
      AlternatingComboExpiration = 0;
      // Turn on kicker for collect shot
      SetKickerStatus(true, 20000);
      goalAdvanced = true;
    }
  }

  return goalAdvanced;
}


void RotateManiaLetters() {
  byte carryOver = SilverballStatus[CurrentPlayer][10];
  for (byte count=10; count<14; count++) {
    SilverballStatus[CurrentPlayer][count] = SilverballStatus[CurrentPlayer][count+1];
  }
  SilverballStatus[CurrentPlayer][14] = carryOver;
}


void IncreaseBonusX() {
  boolean soundPlayed = false;
  if (BonusX[CurrentPlayer] < 5) {
    BonusX[CurrentPlayer] += 1;
    BonusXAnimationStart = CurrentTime;

//    if (BonusX[CurrentPlayer] == 4) {
//      BonusX[CurrentPlayer] += 1;
//    }
  }

  if (!soundPlayed) {
    PlaySoundEffect(SOUND_EFFECT_BONUS_X_INCREASED);
  }

}


int InitGamePlay() {

  if (DEBUG_MESSAGES) {
    Serial.write("Starting game\n\r");
  }

  // The start button has been hit only once to get
  // us into this mode, so we assume a 1-player game
  // at the moment
  BSOS_EnableSolenoidStack();
  BSOS_SetCoinLockout((Credits >= MaximumCredits) ? true : false);
  BSOS_TurnOffAllLamps();
  StopAudio();

  // Reset displays & game state variables
  for (int count = 0; count < 4; count++) {
    // Initialize game-specific variables
    BonusX[count] = 1;
    AddedBonusQualified[count] = 0;
    AddedBonusAchieved[count] = 0;

    TotalSpins[count] = 0;
    Bonus[count] = 0;
    KickerStatus[count] = 0;
    SilverballMode[count] = SILVERBALL_MODE_KNOCK_OUT_LIGHTS;
    for (int letter = 0; letter<15; letter++) {
      SilverballStatus[count][letter] = 0;
    }
  }
  memset(CurrentScores, 0, 4 * sizeof(unsigned long));

  SamePlayerShootsAgain = false;
  CurrentBallInPlay = 1;
  CurrentNumPlayers = 1;
  CurrentPlayer = 0;
  ShowPlayerScores(0xFF, false, false);
  Spinner1kPhase = 0;

  return MACHINE_STATE_INIT_NEW_BALL;
}


int InitNewBall(bool curStateChanged, byte playerNum, int ballNum) {

  // If we're coming into this mode for the first time
  // then we have to do everything to set up the new ball
  if (curStateChanged) {
    BSOS_TurnOffAllLamps();
    BallFirstSwitchHitTime = 0;

    BSOS_SetDisableFlippers(false);
    BSOS_EnableSolenoidStack();
    BSOS_SetDisplayCredits(Credits, true);
    if (CurrentNumPlayers > 1 && (ballNum != 1 || playerNum != 0) && !SamePlayerShootsAgain) PlaySoundEffect(SOUND_EFFECT_PLAYER_1_UP + playerNum);
    SamePlayerShootsAgain = false;

    BSOS_SetDisplayBallInPlay(ballNum);
    BSOS_SetLampState(LAMP_HEAD_TILT, 0);

    if (BallSaveNumSeconds > 0) {
      BSOS_SetLampState(LAMP_SHOOT_AGAIN, 1, 0, 500);
    }

    BallSaveUsed = false;
    BallTimeInTrough = 0;
    NumTiltWarnings = 0;
    LastTiltWarningTime = 0;

    // Initialize game-specific start-of-ball lights & variables
    GameModeStartTime = 0;
    GameModeEndTime = 0;
    GameMode = GAME_MODE_SKILL_SHOT;

    ExtraBallCollected = false;
    SpecialCollected = false;

    // Start appropriate mode music
    if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
      BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 600);
    }
    
    if (BSOS_ReadSingleSwitchState(SW_KICKER_ROLLOVER)) {
      BSOS_PushToTimedSolenoidStack(SOL_KICKBACK_ARM, 6, CurrentTime + 100);
    }

    // Reset progress unless holdover awards
    Bonus[CurrentPlayer] = 0;
    BonusX[CurrentPlayer] = 1;

    ScoreMultiplier = 1;
    LastInlaneHitTime = 0;
    CurrentBonus = Bonus[CurrentPlayer];
    ScoreAdditionAnimation = 0;
    ScoreAdditionAnimationStartTime = 0;
    BonusXAnimationStart = 0;
    LastSpinnerHit = 0;
    //    PlayBackgroundSongBasedOnLevel(StarLevel[CurrentPlayer]);
    WizardScoring = false;
    SilverballPhase = 0;
    ToplanePhase = 0;
    KickerTimeout = 0;
    KickerRolloverWatch = 0;
    LastHorseshoe = 0;
    SuperSkillshotQualified = false;
    ToplaneProgress = 0;
    ToplaneAnimationEnd = 0;
    for (int count=0; count<15; count++) {
      SilverballHighlightEnd[count] = 0;      
    }
    CurrentSilverballWord = GetCurrentSilverballWord();
    LastSilverballSwitchHit = 0;
    LastSilverballLetterAchieved = 0;
    AddedBonusExpiration = 0;
    if (TotalSpins[CurrentPlayer]>20) Spinner1kPhase = 1;
    else Spinner1kPhase = 0;
    AlternatingComboPhase = 0;
    AlternatingCombosHit = 0;
  }


  // We should only consider the ball initialized when
  // the ball is no longer triggering the SW_OUTHOLE
  if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
    return MACHINE_STATE_INIT_NEW_BALL;
  } else {
    return MACHINE_STATE_NORMAL_GAMEPLAY;
  }

}


void PlayBackgroundSongBasedOnBall(byte ballNum) {
  if (ballNum == 1) {
    PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_1);
  } else if (ballNum == BallsPerGame) {
    PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_3);
  } else {
    PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_2);
  }
}

void PlayBackgroundSongBasedOnLevel(byte level) {
  if (level > 2) level = 2;
  PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_1 + level);
}



// This function manages all timers, flags, and lights
int ManageGameMode() {
  int returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

  boolean specialAnimationRunning = false;

  if (TotalSpins[CurrentPlayer] > SPINNER_MAX_GOAL) {
  }

  if (LastHorseshoe && (CurrentTime-LastHorseshoe)>NextHorsehoeTime()) {
    LastHorseshoe = 0;
  }

  for (int count=0; count<15; count++) {
    if (SilverballHighlightEnd[count]!=0 && CurrentTime>SilverballHighlightEnd[count]) {
      SilverballHighlightEnd[count] = 0;
    }
  }

  if (KickerTimeout && CurrentTime>KickerTimeout) {
    SetKickerStatus(false);
    KickerTimeout = 0;
  }

  if (KickerRolloverWatch) {
    if (BSOS_ReadSingleSwitchState(SW_KICKER_ROLLOVER)==0) {
      KickerRolloverWatch = 0;
    } else if (CurrentTime>KickerRolloverWatch) {
      // The kicker has been used, so update the mode
      SetKickerStatus(false);
      PlaySoundEffect(SOUND_EFFECT_KICKER_LAUNCH);
      KickerRolloverWatch = 0;
    }
  }

  if (ToplaneProgress==0x07) {
    IncreaseBonusX();
    ToplaneAnimationEnd = CurrentTime + 2000;
    ToplaneProgress = 0;
  }

  byte currentWizardTimer;

  switch ( (GameMode & GAME_BASE_MODE) ) {
    case GAME_MODE_SKILL_SHOT:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = 0;
      }

      if (BallFirstSwitchHitTime != 0) {
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }

      if (GameModeEndTime != 0 && CurrentTime > GameModeEndTime) {
        ShowPlayerScores(0xFF, false, false);
      }
      break;


    case GAME_MODE_UNSTRUCTURED_PLAY:
      // If this is the first time in this mode
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        SilverballPhase = 0;
      }

      if (AlternatingComboExpiration && (CurrentTime>AlternatingComboExpiration)) {
        AlternatingComboExpiration = 0;
        AlternatingCombosHit = 0;
        AlternatingComboPhase = 0;
      }

      if ((LastHorseshoe && (CurrentTime-LastHorseshoe)<500) || (AlternatingComboPhase==10 && (CurrentTime/1000)%2)) {
        specialAnimationRunning = true;
        ShowLampAnimation(0, 30, CurrentTime-LastHorseshoe, 14, false, false);
      }
      
      break;

    case GAME_MODE_WIZARD_START:
      if (GameModeStartTime == 0) {
        BSOS_TurnOffAllLamps();
        StopAudio();
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + WIZARD_START_DURATION;
        PlaySoundEffect(SOUND_EFFECT_WIZARD_MODE_START);
        StartScoreAnimation(WIZARD_MODE_REWARD_SCORE);
        WizardScoring = true;
      }

      specialAnimationRunning = true;
      ShowLampAnimation(1, 80, CurrentTime, 1, false, false);

      if (CurrentTime > GameModeEndTime) {
        SetGameMode(GAME_MODE_WIZARD);
        LastWizardTimer = 0xFF;
      }
      break;

    case GAME_MODE_WIZARD:
      if (GameModeStartTime == 0) {
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_WIZ);
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + WIZARD_DURATION;
      }

      currentWizardTimer = (byte)((CurrentTime - GameModeStartTime) / 1000);
      if (currentWizardTimer != LastWizardTimer) {
        LastWizardTimer = currentWizardTimer;
        for (byte count = 0; count < 4; count++) {
          if (count != CurrentPlayer) OverrideScoreDisplay(count, WIZARD_DURATION_SECONDS - LastWizardTimer, true);
        }
        PlaySoundEffect(SOUND_EFFECT_SLING_SHOT);
      }

      specialAnimationRunning = true;
      ShowLampAnimation(0, 80, CurrentTime, 14, false, false, 46);

      if (CurrentTime > GameModeEndTime) {
        SetGameMode(GAME_MODE_WIZARD_FINISHED);
      }
      break;

    case GAME_MODE_WIZARD_FINISHED:
      if (GameModeStartTime == 0) {
        BSOS_TurnOffAllLamps();
        StopAudio();
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + WIZARD_FINISHED_DURATION;
        PlaySoundEffect(SOUND_EFFECT_WIZARD_MODE_FINISHED);
        ShowPlayerScores(0xFF, false, false);
      }

      specialAnimationRunning = true;
      ShowLampAnimation(1, 80, CurrentTime, 15, false, false);

      if (CurrentTime > GameModeEndTime) {
        //        PlayBackgroundSongBasedOnLevel(StarLevel[CurrentPlayer]);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
        WizardScoring = false;
      }
      break;

  }

  if ( !specialAnimationRunning && NumTiltWarnings <= MaxTiltWarnings ) {
    ShowBonusLamps();
    ShowBonusXLamps();
    ShowSpinnerLamps();
#if not defined (BALLY_STERN_OS_SOFTWARE_DISPLAY_INTERRUPT)
    BSOS_DataRead(0);
#endif
    ShowShootAgainLamps();
    ShowLaneAndRolloverLamps();
    ShowSilverballLamps();
    ShowKickerLamps();
  }


  // Three types of display modes are shown here:
  // 1) score animation
  // 2) fly-bys
  // 3) normal scores
  if (ScoreAdditionAnimationStartTime != 0) {
    // Score animation
    if ((CurrentTime - ScoreAdditionAnimationStartTime) < 2000) {
      byte displayPhase = (CurrentTime - ScoreAdditionAnimationStartTime) / 60;
      byte digitsToShow = 1 + displayPhase / 6;
      if (digitsToShow > 6) digitsToShow = 6;
      unsigned long scoreToShow = ScoreAdditionAnimation;
      for (byte count = 0; count < (6 - digitsToShow); count++) {
        scoreToShow = scoreToShow / 10;
      }
      if (scoreToShow == 0 || displayPhase % 2) scoreToShow = DISPLAY_OVERRIDE_BLANK_SCORE;
      byte countdownDisplay = (1 + CurrentPlayer) % 4;

      for (byte count = 0; count < 4; count++) {
        if (count == countdownDisplay) OverrideScoreDisplay(count, scoreToShow, false);
        else if (count != CurrentPlayer) OverrideScoreDisplay(count, DISPLAY_OVERRIDE_BLANK_SCORE, false);
      }
    } else {
      byte countdownDisplay = (1 + CurrentPlayer) % 4;
      unsigned long remainingScore = 0;
      if ( (CurrentTime - ScoreAdditionAnimationStartTime) < 5000 ) {
        remainingScore = (((CurrentTime - ScoreAdditionAnimationStartTime) - 2000) * ScoreAdditionAnimation) / 3000;
        if ((remainingScore / 1000) != (LastRemainingAnimatedScoreShown / 1000)) {
          LastRemainingAnimatedScoreShown = remainingScore;
          PlaySoundEffect(SOUND_EFFECT_ANIMATION_TICK);
        }
      } else {
        CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
        remainingScore = 0;
        ScoreAdditionAnimationStartTime = 0;
        ScoreAdditionAnimation = 0;
      }

      for (byte count = 0; count < 4; count++) {
        if (count == countdownDisplay) OverrideScoreDisplay(count, ScoreAdditionAnimation - remainingScore, false);
        else if (count != CurrentPlayer) OverrideScoreDisplay(count, DISPLAY_OVERRIDE_BLANK_SCORE, false);
        else OverrideScoreDisplay(count, CurrentScores[CurrentPlayer] + remainingScore, false);
      }
    }
    if (ScoreAdditionAnimationStartTime) ShowPlayerScores(CurrentPlayer, false, false);
    else ShowPlayerScores(0xFF, false, false);
  } else if (LastSpinnerHit != 0) {
    byte numberToShow = TotalSpins[CurrentPlayer];

    if (numberToShow < 100) {
      ShowFlybyValue(100 - numberToShow, LastSpinnerHit);
    } else {
      ShowPlayerScores(CurrentPlayer, (BallFirstSwitchHitTime == 0) ? true : false, (BallFirstSwitchHitTime > 0 && ((CurrentTime - LastTimeScoreChanged) > 2000)) ? true : false);
    }

    if ((CurrentTime - LastSpinnerHit) > 1000) LastSpinnerHit = 0;
  } else {
    ShowPlayerScores(CurrentPlayer, (BallFirstSwitchHitTime == 0) ? true : false, (BallFirstSwitchHitTime > 0 && ((CurrentTime - LastTimeScoreChanged) > 2000)) ? true : false);
  }

  // Check to see if ball is in the outhole
  if (BSOS_ReadSingleSwitchState(SW_OUTHOLE)) {
    if (BallTimeInTrough == 0) {
      BallTimeInTrough = CurrentTime;
    } else {
      // Make sure the ball stays on the sensor for at least
      // 0.5 seconds to be sure that it's not bouncing
      if ((CurrentTime - BallTimeInTrough) > 500) {

        if (BallFirstSwitchHitTime == 0 && NumTiltWarnings <= MaxTiltWarnings) {
          // Nothing hit yet, so return the ball to the player
          BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime);
          BallTimeInTrough = 0;
          returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
        } else {
          CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
          ScoreAdditionAnimationStartTime = 0;
          ScoreAdditionAnimation = 0;
          ShowPlayerScores(0xFF, false, false);
          // if we haven't used the ball save, and we're under the time limit, then save the ball
          if (!BallSaveUsed && ((CurrentTime - BallFirstSwitchHitTime)) < ((unsigned long)BallSaveNumSeconds * 1000)) {
            BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
            BallSaveUsed = true;
            //            PlaySoundEffect(SOUND_EFFECT_SHOOT_AGAIN);
            BSOS_SetLampState(LAMP_SHOOT_AGAIN, 0);
            BallTimeInTrough = CurrentTime;
            returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
          } else {
            ShowPlayerScores(0xFF, false, false);
            PlayBackgroundSong(SOUND_EFFECT_NONE);
            StopAudio();

            if (CurrentBallInPlay < BallsPerGame) PlaySoundEffect(SOUND_EFFECT_BALL_OVER);
            returnState = MACHINE_STATE_COUNTDOWN_BONUS;
          }
        }
      }
    }
  } else {
    BallTimeInTrough = 0;
  }

  return returnState;
}



unsigned long CountdownStartTime = 0;
unsigned long LastCountdownReportTime = 0;
unsigned long BonusCountDownEndTime = 0;
byte BonusCountdownProgress;

int CountdownBonus(boolean curStateChanged) {

  // If this is the first time through the countdown loop
  if (curStateChanged) {

    Bonus[CurrentPlayer] = CurrentBonus;
    CountdownStartTime = CurrentTime;
    ShowBonusLamps();

    LastCountdownReportTime = CountdownStartTime;
    BonusCountDownEndTime = 0xFFFFFFFF;
    BonusCountdownProgress = 0; 
  }

  unsigned long countdownDelayTime = 80/* - (CurrentBonus * 3)*/;
  // Show bonus countdown twice as fast if we're tilted
  if (NumTiltWarnings>MaxTiltWarnings) countdownDelayTime /= 2;

  if ((CurrentTime - LastCountdownReportTime) > countdownDelayTime) {

    if (BonusCountdownProgress<10) {
      // Show bonus collect for letters on backbox
      while ( (BonusCountdownProgress<10) && SilverballStatus[CurrentPlayer][BonusCountdownProgress]<0x10) {
        BonusCountdownProgress += 1;
      }
      if (BonusCountdownProgress<10) {
        if (NumTiltWarnings <= MaxTiltWarnings) {
          PlaySoundEffect(SOUND_EFFECT_BONUS_COUNT_LO);
          CurrentScores[CurrentPlayer] += 1000 * ((unsigned long)BonusX[CurrentPlayer]) * ((unsigned long)SilverballStatus[CurrentPlayer][BonusCountdownProgress]/16);
        }
        BSOS_SetLampState(HeadSilverballLampIndex[BonusCountdownProgress], 1, 1);
        BonusCountdownProgress += 1;
      }
    } else if (BonusCountdownProgress<25) {
      while (BonusCountdownProgress<25 && (SilverballStatus[CurrentPlayer][BonusCountdownProgress-10]&0x0F)==0) {
        BonusCountdownProgress += 1;
      }
      if (BonusCountdownProgress<25) {
        if (NumTiltWarnings <= MaxTiltWarnings) {
          if (BonusCountdownProgress<20 && (SilverballStatus[CurrentPlayer][BonusCountdownProgress-10]&0xF0)<(SilverballMode[CurrentPlayer]*16)) {
            SilverballStatus[CurrentPlayer][BonusCountdownProgress-10] = (SilverballStatus[CurrentPlayer][BonusCountdownProgress-10]&0x0F) | (SilverballMode[CurrentPlayer]*16);
            BSOS_SetLampState(HeadSilverballLampIndex[BonusCountdownProgress-10], 1, 0, 100);
          }
          PlaySoundEffect(SOUND_EFFECT_BONUS_COUNT_HI);
          CurrentScores[CurrentPlayer] += 1000 * ((unsigned long)BonusX[CurrentPlayer]);
        }
        BSOS_SetLampState(LAMP_PLAYFIELD_SPELLOUT_S + (BonusCountdownProgress-10), 0);
        BonusCountdownProgress += 1;
      }      
    } else if (BonusCountdownProgress<35) {
      if (AddedBonusAchieved[CurrentPlayer]>=60) {
        BSOS_SetLampState(LAMP_KICKER_SPECIAL, 1, 0, 100);
        BonusCountdownProgress += 1;
      } else {
        BonusCountdownProgress = 35;
        BSOS_SetLampState(LAMP_KICKER_SPECIAL, 0);
      }
    } else if (BonusCountdownProgress<45) {
      BSOS_SetLampState(LAMP_KICKER_SPECIAL, 0);
      if (AddedBonusAchieved[CurrentPlayer]>=30) {
        if (BonusCountdownProgress==35) PlaySoundEffect(SOUND_EFFECT_BONUS_COUNT_HI);
        BonusCountdownProgress += 1;
        BSOS_SetLampState(LAMP_30K_BONUS, 1, 0, 100);
      } else {
        BonusCountdownProgress = 45;
        BSOS_SetLampState(LAMP_30K_BONUS, 0);
      }
    } else if (BonusCountdownProgress<55) {
      BSOS_SetLampState(LAMP_30K_BONUS, 0);
      if (AddedBonusAchieved[CurrentPlayer]>=15) {
        if (BonusCountdownProgress==45) PlaySoundEffect(SOUND_EFFECT_BONUS_COUNT_HI);
        BonusCountdownProgress += 1;
        BSOS_SetLampState(LAMP_15K_BONUS, 1, 0, 100);
      } else {
        BonusCountdownProgress = 55;
        BSOS_SetLampState(LAMP_15K_BONUS, 0);
      }
    } else if (BonusCountDownEndTime == 0xFFFFFFFF) {
      BSOS_SetLampState(LAMP_15K_BONUS, 0);
      BonusCountDownEndTime = CurrentTime + 1000;
    }
    LastCountdownReportTime = CurrentTime;
  }

  if (CurrentTime > BonusCountDownEndTime) {

    // Reset any lights & variables of goals that weren't completed
    BonusCountDownEndTime = 0xFFFFFFFF;
    return MACHINE_STATE_BALL_OVER;
  }

  return MACHINE_STATE_COUNTDOWN_BONUS;
}



void CheckHighScores() {
  unsigned long highestScore = 0;
  int highScorePlayerNum = 0;
  for (int count = 0; count < CurrentNumPlayers; count++) {
    if (CurrentScores[count] > highestScore) highestScore = CurrentScores[count];
    highScorePlayerNum = count;
  }

  if (highestScore > HighScore) {
    HighScore = highestScore;
    if (HighScoreReplay) {
      AddCredit(false, 3);
      BSOS_WriteULToEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE) + 3);
    }
    BSOS_WriteULToEEProm(BSOS_HIGHSCORE_EEPROM_START_BYTE, highestScore);
    BSOS_WriteULToEEProm(BSOS_TOTAL_HISCORE_BEATEN_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_HISCORE_BEATEN_START_BYTE) + 1);

    for (int count = 0; count < 4; count++) {
      if (count == highScorePlayerNum) {
        BSOS_SetDisplay(count, CurrentScores[count], true, 2);
      } else {
        BSOS_SetDisplayBlank(count, 0x00);
      }
    }

    BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
    BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime + 300, true);
    BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime + 600, true);
  }
}


unsigned long MatchSequenceStartTime = 0;
unsigned long MatchDelay = 150;
byte MatchDigit = 0;
byte NumMatchSpins = 0;
byte ScoreMatches = 0;

int ShowMatchSequence(boolean curStateChanged) {
  if (!MatchFeature) return MACHINE_STATE_ATTRACT;

  if (curStateChanged) {
    MatchSequenceStartTime = CurrentTime;
    MatchDelay = 1500;
    MatchDigit = CurrentTime % 10;
    NumMatchSpins = 0;
    BSOS_SetLampState(LAMP_HEAD_MATCH, 1, 0);
    BSOS_SetDisableFlippers();
    ScoreMatches = 0;
  }

  if (NumMatchSpins < 40) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      MatchDigit += 1;
      if (MatchDigit > 9) MatchDigit = 0;
      //PlaySoundEffect(10+(MatchDigit%2));
      PlaySoundEffect(SOUND_EFFECT_MATCH_SPIN);
      BSOS_SetDisplayBallInPlay((int)MatchDigit * 10);
      MatchDelay += 50 + 4 * NumMatchSpins;
      NumMatchSpins += 1;
      BSOS_SetLampState(LAMP_HEAD_MATCH, NumMatchSpins % 2, 0);

      if (NumMatchSpins == 40) {
        BSOS_SetLampState(LAMP_HEAD_MATCH, 0);
        MatchDelay = CurrentTime - MatchSequenceStartTime;
      }
    }
  }

  if (NumMatchSpins >= 40 && NumMatchSpins <= 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      if ( (CurrentNumPlayers > (NumMatchSpins - 40)) && ((CurrentScores[NumMatchSpins - 40] / 10) % 10) == MatchDigit) {
        ScoreMatches |= (1 << (NumMatchSpins - 40));
        AddSpecialCredit();
        MatchDelay += 1000;
        NumMatchSpins += 1;
        BSOS_SetLampState(LAMP_HEAD_MATCH, 1);
      } else {
        NumMatchSpins += 1;
      }
      if (NumMatchSpins == 44) {
        MatchDelay += 5000;
      }
    }
  }

  if (NumMatchSpins > 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      return MACHINE_STATE_ATTRACT;
    }
  }

  for (int count = 0; count < 4; count++) {
    if ((ScoreMatches >> count) & 0x01) {
      // If this score matches, we're going to flash the last two digits
      if ( (CurrentTime / 200) % 2 ) {
        BSOS_SetDisplayBlank(count, BSOS_GetDisplayBlank(count) & 0x0F);
      } else {
        BSOS_SetDisplayBlank(count, BSOS_GetDisplayBlank(count) | 0x30);
      }
    }
  }

  return MACHINE_STATE_MATCH_MODE;
}





int RunGamePlayMode(int curState, boolean curStateChanged) {
  int returnState = curState;
  unsigned long scoreAtTop = CurrentScores[CurrentPlayer];

  // Very first time into gameplay loop
  if (curState == MACHINE_STATE_INIT_GAMEPLAY) {
    returnState = InitGamePlay();
  } else if (curState == MACHINE_STATE_INIT_NEW_BALL) {
    returnState = InitNewBall(curStateChanged, CurrentPlayer, CurrentBallInPlay);
  } else if (curState == MACHINE_STATE_NORMAL_GAMEPLAY) {
    returnState = ManageGameMode();
  } else if (curState == MACHINE_STATE_COUNTDOWN_BONUS) {
    returnState = CountdownBonus(curStateChanged);
    ShowPlayerScores(0xFF, false, false);
  } else if (curState == MACHINE_STATE_BALL_OVER) {
    BSOS_SetDisplayCredits(Credits);
    SetKickerStatus(false);

    if (SamePlayerShootsAgain) {
      PlaySoundEffect(SOUND_EFFECT_SHOOT_AGAIN);
      returnState = MACHINE_STATE_INIT_NEW_BALL;
    } else {

      CurrentPlayer += 1;
      if (CurrentPlayer >= CurrentNumPlayers) {
        CurrentPlayer = 0;
        CurrentBallInPlay += 1;
      }

      scoreAtTop = CurrentScores[CurrentPlayer];

      if (CurrentBallInPlay > BallsPerGame) {
        CheckHighScores();
        PlaySoundEffect(SOUND_EFFECT_GAME_OVER);
        for (int count = 0; count < CurrentNumPlayers; count++) {
          BSOS_SetDisplay(count, CurrentScores[count], true, 2);
        }

        returnState = MACHINE_STATE_MATCH_MODE;
      }
      else returnState = MACHINE_STATE_INIT_NEW_BALL;
    }
  } else if (curState == MACHINE_STATE_MATCH_MODE) {
    returnState = ShowMatchSequence(curStateChanged);
  }

  byte switchHit;
  //  ScoreMultiplier = 1 + CountBits(GameMode & 0xF0);

  if (NumTiltWarnings <= MaxTiltWarnings) {
    while ( (switchHit = BSOS_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {

      if (DEBUG_MESSAGES) {
        char buf[128];
        sprintf(buf, "Switch Hit = %d\n", switchHit);
        Serial.write(buf);
      }

      if (WizardScoring) {
        if (switchHit != SW_SLAM && switchHit != SW_TILT) {
          //          PlaySoundEffect(SOUND_EFFECT_WIZARD_SCORE);
          CurrentScores[CurrentPlayer] += WIZARD_SWITCH_SCORE * ScoreMultiplier;
        }
      }

      switch (switchHit) {
        case SW_SLAM:
          //          BSOS_DisableSolenoidStack();
          //          BSOS_SetDisableFlippers(true);
          //          BSOS_TurnOffAllLamps();
          //          BSOS_SetLampState(GAME_OVER, 1);
          //          delay(1000);
          //          return MACHINE_STATE_ATTRACT;
          break;
        case SW_TILT:
          // This should be debounced
          if ((CurrentTime - LastTiltWarningTime) > TILT_WARNING_DEBOUNCE_TIME) {
            LastTiltWarningTime = CurrentTime;
            NumTiltWarnings += 1;
            if (NumTiltWarnings > MaxTiltWarnings) {
              BSOS_DisableSolenoidStack();
              BSOS_SetDisableFlippers(true);
              BSOS_TurnOffAllLamps();
              StopAudio();
              PlaySoundEffect(SOUND_EFFECT_TILT);
              BSOS_SetLampState(LAMP_HEAD_TILT, 1);
            }
            PlaySoundEffect(SOUND_EFFECT_TILT_WARNING);
          }
          break;
        case SW_SELF_TEST_SWITCH:
          returnState = MACHINE_STATE_TEST_LIGHTS;
          SetLastSelfTestChangedTime(CurrentTime);
          break;
        //        case SW_10_PTS:
        case SW_TOP_RIGHT_ROLLOVER:
        case SW_TOP_LEFT_ROLLOVER:
          if (switchHit==SW_TOP_LEFT_ROLLOVER) ToplaneProgress |= 0x01;
          else ToplaneProgress |= 0x04;
          
          if (GameMode==GAME_MODE_SKILL_SHOT && ToplanePhase==1) {
            SpotSilverballLetter();
            PlaySoundEffect(SOUND_EFFECT_SKILL_SHOT);
          } else {
            PlaySoundEffect(SOUND_EFFECT_ROLLOVER);
            CurrentScores[CurrentPlayer] += 100 * ScoreMultiplier;
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_TOP_CENTER_ROLLOVER:
          ToplaneProgress |= 0x02;
          if (GameMode==GAME_MODE_SKILL_SHOT && ToplanePhase==0) {
            PlaySoundEffect(SOUND_EFFECT_SKILL_SHOT);
            SpotSilverballLetter();
            StartScoreAnimation(5000 * ScoreMultiplier);
            if (GameMode==GAME_MODE_SKILL_SHOT) {
              SetKickerStatus(true, 15000);
            }
          } else {
            CurrentScores[CurrentPlayer] += 100 * ScoreMultiplier;
            PlaySoundEffect(SOUND_EFFECT_ROLLOVER);
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_LEFT_SPINNER:
        case SW_RIGHT_SPINNER:
          TotalSpins[CurrentPlayer] += 1;
          if (AdvanceAlternatingCombo(switchHit)) {
            CurrentScores[CurrentPlayer] += 1500 * ScoreMultiplier;
            PlaySoundEffect(SOUND_EFFECT_SPINNER_HIGH);
          } else if (Spinner1kPhase==1 && switchHit==SW_LEFT_SPINNER) {
            CurrentScores[CurrentPlayer] += 1000 * ScoreMultiplier;
            PlaySoundEffect(SOUND_EFFECT_SPINNER_HIGH);
          } else if (Spinner1kPhase==2 && switchHit==SW_RIGHT_SPINNER) {
            CurrentScores[CurrentPlayer] += 1000 * ScoreMultiplier;
            PlaySoundEffect(SOUND_EFFECT_SPINNER_HIGH);
          } else {
            CurrentScores[CurrentPlayer] += 100 * ScoreMultiplier;
            PlaySoundEffect(SOUND_EFFECT_SPINNER_LOW);
          }
          break;
        case SW_KICKER_ROLLOVER:
          KickerRolloverWatch = CurrentTime + 1000;
          if (KickerStatus[CurrentPlayer]) {
            if (AddedBonusQualified[CurrentPlayer]) {
              PlaySoundEffect(SOUND_EFFECT_ADDED_BONUS_COLLECT);
              AddedBonusAchieved[CurrentPlayer] = AddedBonusQualified[CurrentPlayer];
              StartScoreAnimation(5000 * ScoreMultiplier * ((unsigned long)AddedBonusQualified[CurrentPlayer]));
              AddedBonusQualified[CurrentPlayer] = 0;
            } else {
              StartScoreAnimation(5000 * ScoreMultiplier);
              PlaySoundEffect(SOUND_EFFECT_KICKER_WATCH);
            }
          } else {
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_HOOP_ROLLOVER_BUTTON:
          if (GameMode==GAME_MODE_SKILL_SHOT) {
            StartScoreAnimation(20000 * ScoreMultiplier);
            PlaySoundEffect(SOUND_EFFECT_SKILL_SHOT);
            SuperSkillshotQualified = true;
          } else {
            if (AdvanceAlternatingCombo(switchHit)) {
              StartScoreAnimation(15000 * ScoreMultiplier);
              PlaySoundEffect(SOUND_EFFECT_ADDED_BONUS_QUALIFIED);
            } else {
              StartScoreAnimation(5000 * ScoreMultiplier);
              PlaySoundEffect(SOUND_EFFECT_HORSESHOE);
            }
          }
          SetKickerStatus(true);
          LastHorseshoe = CurrentTime;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_LEFT_BUMPER:
        case SW_RIGHT_BUMPER:
        case SW_CENTER_BUMPER:
          CurrentScores[CurrentPlayer] += 100 * ScoreMultiplier;
          PlaySoundEffect(SOUND_EFFECT_BUMPER_HIT);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_RIGHT_A_TARGET:
        case SW_LEFT_B_TARGET:
        case SW_LEFT_R_TARGET:
        case SW_LEFT_E_TARGET:
        case SW_TOP_V_TARGET:
        case SW_TOP_L_TARGET:
        case SW_TOP_I_TARGET:
        case SW_TOP_S_TARGET:
        case SW_RIGHT_OUTLANE_A:
        case SW_RIGHT_INLANE_I:
        case SW_CENTER_TARGET:
        case SW_LEFT_INLANE_A:
        case SW_LEFT_OUTLANE_M:
        case SW_LOWER_L_SIDE_TARGET:
        case SW_UPPER_L_SIDE_TARGET:
          HandleSilverballHit(switchHit);
          AdvanceAlternatingCombo(switchHit);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_REBOUNDS_AND_TOP:
          ToplanePhase = (ToplanePhase+1)%2;         
          if (Spinner1kPhase) {
            Spinner1kPhase += 1;
            if (Spinner1kPhase>2) Spinner1kPhase = 1;
          }
          if (SilverballMode[CurrentPlayer]<5) RotateManiaLetters();
          PlaySoundEffect(SOUND_EFFECT_50PT_SWITCH);
          CurrentScores[CurrentPlayer] += (ScoreMultiplier) * 50;          
          break;
        case SW_LEFT_SLINGSHOT:
        case SW_RIGHT_SLINGSHOT:
          CurrentScores[CurrentPlayer] += 10 * ScoreMultiplier;
          PlaySoundEffect(SOUND_EFFECT_SLING_SHOT);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_COIN_1:
        case SW_COIN_2:
        case SW_COIN_3:
          AddCoinToAudit(switchHit);
          AddCredit(true, 1);
          break;
        case SW_CREDIT_RESET:
          if (CurrentBallInPlay < 2) {
            // If we haven't finished the first ball, we can add players
            AddPlayer();
          } else {
            // If the first ball is over, pressing start again resets the game
            if (Credits >= 1 || FreePlayMode) {
              if (!FreePlayMode) {
                Credits -= 1;
                BSOS_WriteByteToEEProm(BSOS_CREDITS_EEPROM_BYTE, Credits);
                BSOS_SetDisplayCredits(Credits);
              }
              returnState = MACHINE_STATE_INIT_GAMEPLAY;
            }
          }
          if (DEBUG_MESSAGES) {
            Serial.write("Start game button pressed\n\r");
          }
          break;
      }
    }
  } else {
    // We're tilted, so just wait for outhole
    while ( (switchHit = BSOS_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
      switch (switchHit) {
        case SW_SELF_TEST_SWITCH:
          returnState = MACHINE_STATE_TEST_LIGHTS;
          SetLastSelfTestChangedTime(CurrentTime);
          break;
        case SW_COIN_1:
        case SW_COIN_2:
        case SW_COIN_3:
          AddCoinToAudit(switchHit);
          AddCredit(true, 1);
          break;
      }
    }
  }

  if (!ScrollingScores && CurrentScores[CurrentPlayer] > BALLY_STERN_OS_MAX_DISPLAY_SCORE) {
    CurrentScores[CurrentPlayer] -= BALLY_STERN_OS_MAX_DISPLAY_SCORE;
  }

  if (scoreAtTop != CurrentScores[CurrentPlayer]) {
    LastTimeScoreChanged = CurrentTime;
    if (!TournamentScoring) {
      for (int awardCount = 0; awardCount < 3; awardCount++) {
        if (AwardScores[awardCount] != 0 && scoreAtTop < AwardScores[awardCount] && CurrentScores[CurrentPlayer] >= AwardScores[awardCount]) {
          // Player has just passed an award score, so we need to award it
          if (((ScoreAwardReplay >> awardCount) & 0x01)) {
            AddSpecialCredit();
          } else if (!ExtraBallCollected) {
            AwardExtraBall();
          }
        }
      }
    }

  }

  return returnState;
}


void loop() {

  BSOS_DataRead(0);

  CurrentTime = millis();
  int newMachineState = MachineState;

  if (MachineState < 0) {
    newMachineState = RunSelfTest(MachineState, MachineStateChanged);
  } else if (MachineState == MACHINE_STATE_ATTRACT) {
    newMachineState = RunAttractMode(MachineState, MachineStateChanged);
  } else {
    newMachineState = RunGamePlayMode(MachineState, MachineStateChanged);
  }

  if (newMachineState != MachineState) {
    MachineState = newMachineState;
    MachineStateChanged = true;
  } else {
    MachineStateChanged = false;
  }

#ifdef BALLY_STERN_OS_USE_DASH51
  byte curSound = GetFromSoundQueue(CurrentTime);
  if (curSound!=0xFF) {
    BSOS_PlaySoundDash51(curSound);
  }
#endif


  BSOS_ApplyFlashToLamps(CurrentTime);
  BSOS_UpdateTimedSolenoidStack(CurrentTime);

}
