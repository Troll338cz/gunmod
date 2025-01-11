#ifndef DISPLACERBALL_H
#define DISPLACERBALL_H

class CDisplacerBall : public CBaseEntity
{
public:
	void Spawn( void );

	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngles);
	static void SelfCreate(entvars_t *pevOwner, Vector vecStart);

	void Touch(CBaseEntity *pOther);
	void EXPORT ExplodeThink( void );
	void EXPORT KillThink( void );
	void Circle( void );

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	CBeam* m_pBeam[8];

	void EXPORT FlyThink( void );
	void ClearBeams( void );
	void ArmBeam( int iSide );

	int m_iBeams;
	int m_iTrail;

	CBaseEntity *pRemoveEnt;
	int m_iGravUse;

	float TouchGravGun( CBaseEntity *attacker, int stage)
	{
		pev->owner = attacker->edict();
		if( stage == 2 )
		{
			m_iGravUse = 1;
			Touch( attacker );
		}

		return 300;
	}
};

#endif