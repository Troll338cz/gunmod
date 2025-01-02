#ifndef SHOCKBEAM_H
#define SHOCKBEAM_H

//=========================================================
// shock - Shockrifle projectile
//=========================================================
class CShock : public CBaseAnimating
{
public:
	void Spawn(void);
	void Precache();

	static void Shoot(entvars_t *pevOwner, const Vector angles, const Vector vecStart, const Vector vecVelocity);
	void Touch(CBaseEntity *pOther);
	void EXPORT FlyThink();

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
	
	virtual float TouchGravGun( CBaseEntity *attacker, int stage )
	{
		pev->owner = attacker->edict();
		if( pev->flags & FL_IMMUNE_LAVA ) // Remove supercharged flag if someone gravguns it
			pev->flags &= ~FL_IMMUNE_LAVA;

		if( stage == 2 )
		{
			UTIL_MakeVectors( attacker->pev->v_angle + attacker->pev->punchangle);
			Vector atarget = UTIL_VecToAngles(gpGlobals->v_forward);
			pev->angles.x = UTIL_AngleMod(pev->angles.x);
			pev->angles.y = UTIL_AngleMod(pev->angles.y);
			pev->angles.z = UTIL_AngleMod(pev->angles.z);
			atarget.x = UTIL_AngleMod(atarget.x);
			atarget.y = UTIL_AngleMod(atarget.y);
			atarget.z = UTIL_AngleMod(atarget.z);
			pev->avelocity.x = UTIL_AngleDiff(atarget.x, pev->angles.x) * 10;
			pev->avelocity.y = UTIL_AngleDiff(atarget.y, pev->angles.y) * 10;
			pev->avelocity.z = UTIL_AngleDiff(atarget.z, pev->angles.z) * 10;
		}

		return 2000;
	}

	void CreateEffects();
	void ClearEffects();

	CBeam *m_pBeam;
	CBeam *m_pNoise;
	CSprite *m_pSprite;
	int m_iTrail;
};

#endif
