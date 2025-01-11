#ifndef CSANTAHAT_H
#define CSANTAHAT_H

class CSantaHat : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	static CSantaHat *CreateSantaHat(CBaseEntity *aiment);
};

#endif // CSANTAHAT_H
