#ifndef PORT_H
#define PORT_H

class CPortalCreator : public CSprite
{
public:
        void Spawn(void);
        void Precache();
        static void Shoot(edict_t *pevOwner, const Vector angles, const Vector vecStart, const Vector vecVelocity, BOOL blue);
        void Touch(CBaseEntity *pOther);
        void EXPORT FlyThink();
	void OpenError();
	static void ClosePortals(edict_t *pevOwner);

        virtual float TouchGravGun( CBaseEntity *attacker, int stage )
        {
                return 1000;
        }

        void CreateEffects();

        CSprite *m_pSprite;
        int m_iTrail;
	int m_iBeam;
	int m_iBlue;
	int m_iOrange;
        BOOL m_bBlue = FALSE;
};

#endif // PORTAL_H
