#ifndef PROP_PORTAL_H
#define PROP_PORTAL_H

#define PORTAL_HEIGHT 72
#define PORTAL_WIDTH 46

class CPortal : public CBaseEntity
{
public:
	void Spawn(void);
	void Precache();
	static void OpenPortal(edict_t *pevOwner, const Vector angles, const Vector vecStart, CBaseEntity *touchEnt, BOOL blue);
	void ClosePortal();

	void PortalTouch(CBaseEntity *pOther);

	BOOL m_bBlue = FALSE;
};

#endif // PROP_PORTAL_H
