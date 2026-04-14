// music.h


void EnableMusic( MBoolean on );
void PauseMusic( void );
void ResumeMusic( void );
void FastMusic( void );
void SlowMusic( void );
void UpdateMusic( void );
void FadeOutMusic( int durationTicks );
int GetCurrentMusic( void );
void ChooseMusic( short which );
void ShutdownMusic();


extern MBoolean musicOn;
extern int      musicSelection;

