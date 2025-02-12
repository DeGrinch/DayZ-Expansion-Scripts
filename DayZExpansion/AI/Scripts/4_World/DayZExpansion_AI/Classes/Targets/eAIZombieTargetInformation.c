class eAIZombieTargetInformation: eAIEntityTargetInformation
{
	private ZombieBase m_Zombie;

	bool m_Crawling;

	void eAIZombieTargetInformation(EntityAI target)
	{
		Class.CastTo(m_Zombie, target);
	}

	override bool IsCrawling()
	{
		return m_Crawling;
	}

	override vector GetAimOffset(eAIBase ai = null)
	{
		string boneName;
		if (ai && Weapon_Base.Cast(ai.GetHumanInventory().GetEntityInHands()))
			boneName = "neck";
		else
			boneName = "spine3";  //! Aim lower for melee
		vector pos = m_Zombie.GetBonePositionWS(m_Zombie.GetBoneIndexByName(boneName));
		pos = pos - m_Zombie.GetPosition();
		return pos;
	}

	// https://www.desmos.com/calculator/r4mqu91qff
	override float CalculateThreat(eAIBase ai = null)
	{
		if (m_Zombie.IsDamageDestroyed())
			return 0.0;

		float levelFactor;

		// TODO: check to see if ::GetMindState() returns int of 0->4
		DayZInfectedInputController diip = m_Zombie.GetInputController();
		int level = diip.GetMindState();
		switch (level)
		{
		case DayZInfectedConstants.MINDSTATE_CALM:
			levelFactor = 0.00;
			break;
		case DayZInfectedConstants.MINDSTATE_DISTURBED:
			levelFactor = 0.25;
			break;
		case DayZInfectedConstants.MINDSTATE_ALERTED:
			levelFactor = 0.50;
			break;
		case DayZInfectedConstants.MINDSTATE_CHASE:
			levelFactor = 1.0;
			break;
		case DayZInfectedConstants.MINDSTATE_FIGHT:
			levelFactor = 2.00;
			break;
		}

		if (ai)
		{
#ifdef DIAG
			auto hitch = new EXHitch(ai.ToString() + " eAIZombieTargetInformation::CalculateThreat ", 20000);
#endif

			// the further away the zombie, the less likely it will be a threat
			float distance = GetDistance(ai, true) + 0.1;
			levelFactor *= 10 / distance;
			if (levelFactor > 1.0)
				levelFactor = Math.Pow(levelFactor, 2.0);

			if (diip.GetTargetEntity() == ai)
			{
				levelFactor *= 2.0;
				auto hands = ai.GetHumanInventory().GetEntityInHands();
				if (hands)
					eAIPlayerTargetInformation.AdjustThreatLevelBasedOnWeapon(hands, distance, levelFactor);
			}

			levelFactor *= ai.Expansion_GetVisibility(distance);
		}

		return Math.Clamp(levelFactor, 0.0, 1000000.0);
	}

	override bool ShouldRemove(eAIBase ai = null)
	{
		return GetThreat(ai) <= 0.1;
	}
};
