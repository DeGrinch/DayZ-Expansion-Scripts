/**
 * ExpansionQuestMenu.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionQuestMenu: ExpansionScriptViewMenu
{
	private autoptr ExpansionQuestMenuController m_QuestMenuController;
	private ExpansionQuestModule m_QuestModule;
	private autoptr array<ref ExpansionQuestConfig> m_Quests = new array<ref ExpansionQuestConfig>;
	private autoptr ExpansionQuestConfig m_SelectedQuest;
	private autoptr ExpansionQuestRewardConfig m_SelectedReward;

	private Widget QuestListPanel;
	private Widget QuestDetailsPanel;
	private ButtonWidget Accept;
	private TextWidget AcceptLable;
	private ButtonWidget Complete;
	private TextWidget CompleteLable;
	private ButtonWidget Cancel;
	private TextWidget CancelLable;
	private ButtonWidget Close;
	private TextWidget CloseLable;
	private WrapSpacerWidget ButtonsPanel;
	
	private TextWidget Reward;
	private Widget RewardPanel;
	private ScrollWidget ObjectiveSectionScroller;
	private RichTextWidget Objective;
	private WrapSpacerWidget QuestListContent;
	private Widget DefaultPanel;
	private Widget Humanity;

	void ExpansionQuestMenu()
	{
		Class.CastTo(m_QuestModule, CF_ModuleCoreManager.Get(ExpansionQuestModule));
		Class.CastTo(m_QuestMenuController, GetController());

		m_QuestModule.GetQuestMenuSI().Insert(SetQuests);
		QuestListPanel.Show(true);
		QuestDetailsPanel.Show(false);
	}

	void ~ExpansionQuestMenu()
	{
		m_QuestModule.GetQuestMenuSI().Remove(SetQuests);
		m_QuestMenuController.Quests.Clear();
		m_Quests.Clear();
		
		m_SelectedQuest = NULL;
		m_SelectedReward = NULL;
	}

	override string GetLayoutFile()
	{
		return "DayZExpansion/Quests/GUI/layouts/quests/expansion_quest_menu.layout";
	}

	override typename GetControllerType()
	{
		return ExpansionQuestMenuController;
	}

	void SetQuests(array<ref ExpansionQuestConfig> quests, ExpansionQuestNPCData questNPCData)
	{
		QuestDebug(ToString() + "::SetQuests - Start");
		QuestDebug(ToString() + "::SetQuests - Quest: " + quests.ToString());

		if (!m_QuestModule)
			return;

		m_SelectedReward = NULL;
		ButtonsPanel.Show(false);
		
		m_QuestMenuController.QuestNPCName = questNPCData.GetNPCName();
		m_QuestMenuController.NotifyPropertyChanged("QuestNPCName");

		if (quests.Count() > 0)
		{
			QuestListContent.Show(true);
			DefaultPanel.Show(false);

			for (int i = 0; i < quests.Count(); i++)
			{
				ExpansionQuestConfig quest = quests[i];
				if (!quest || quest.IsAchivement())
					continue;
				
			#ifdef EXPANSIONMODHARDLINE
				if (quest.IsBanditQuest() || quest.IsHeroQuest() && !GetExpansionSettings().GetHardline().UseHumanity)
					continue;
			#endif

				m_Quests.Insert(quest);
				ExpansionQuestMenuListEntry questEntry = new ExpansionQuestMenuListEntry(quest, this);
				m_QuestMenuController.Quests.Insert(questEntry);
			}
		}
		else if (quests.Count() == 0)
		{
			if (!questNPCData)
				return;

			QuestListContent.Show(false);
			DefaultPanel.Show(true);

			m_QuestMenuController.DefaultText = questNPCData.GetDefaultNPCText();
			m_QuestMenuController.NotifyPropertyChanged("DefaultText");
		}

		QuestDebug(ToString() + "::SetQuests - End");
	}

	void OnAcceptButtonClick()
	{
		if (!m_SelectedQuest)
			return;

		m_QuestModule.AcceptQuestClient(m_SelectedQuest);
		CloseMenu();
	}

	void OnCancelButtonClick()
	{
		QuestListPanel.Show(true);
		ButtonsPanel.Show(false);
		QuestDetailsPanel.Show(false);
	}

	void OnCompleteButtonClick()
	{
		if (!m_SelectedQuest)
			return;
		
		if (m_SelectedQuest.NeedToSelectReward())
		{
			if (m_SelectedReward)
			{
				m_QuestModule.RequestTurnInQuestClient(m_SelectedQuest.GetID(), true, m_SelectedReward);
			}
			else
			{
				StringLocaliser title = new StringLocaliser("STR_EXPANSION_QUEST_TITLE", m_SelectedQuest.GetTitle());
				StringLocaliser text = new StringLocaliser("STR_EXPANSION_QUEST_MENU_ERROR_REWARD");
				ExpansionNotification(title, text, EXPANSION_NOTIFICATION_ICON_INFO, COLOR_EXPANSION_NOTIFICATION_ERROR).Create();
			}
		}
		else
		{
			m_QuestModule.RequestTurnInQuestClient(m_SelectedQuest.GetID());
		}

		ButtonsPanel.Show(false);
		CloseMenu();
	}

	void SetQuest(ExpansionQuestConfig quest)
	{
		QuestDebug(ToString() + "::SetQuest - Start");

		if (!m_QuestModule)
			return;

		bool hasQuestState = false;
		int questState = m_QuestModule.GetClientQuestData().GetQuestStateByQuestID(quest.GetID());
		if (questState > ExpansionQuestState.NONE)
			hasQuestState = true;

		QuestDebug(ToString() + "::SetQuest - Quest state: " + questState);
		QuestListPanel.Show(false);
		ButtonsPanel.Show(true);
		QuestDetailsPanel.Show(true);

		m_SelectedQuest = quest;
		m_QuestMenuController.QuestTitle = quest.GetTitle();
		
		m_SelectedQuest.QuestDebug();

		string description;
		if (!hasQuestState || hasQuestState && questState == ExpansionQuestState.NONE)
		{
			description = quest.GetDescriptions()[0];
			Accept.Show(true);
			Complete.Show(false);
		}
		else if (hasQuestState && questState == ExpansionQuestState.STARTED)
		{
			description = quest.GetDescriptions()[1];
			Accept.Show(false);
			Complete.Show(false);
		}
		else if (hasQuestState && questState == ExpansionQuestState.CAN_TURNIN)
		{
			description = quest.GetDescriptions()[2];
			Accept.Show(false);
			Complete.Show(true);
		}

		StringLocaliser descriptiontext = new StringLocaliser(description, GetGame().GetPlayer().GetIdentity().GetName());
		m_QuestMenuController.QuestDescription = descriptiontext.Format();
		m_QuestMenuController.QuestObjective = quest.GetObjectiveText();

		m_QuestMenuController.NotifyPropertiesChanged({"QuestTitle", "QuestDescription", "QuestObjective"});

		m_QuestMenuController.RewardEntries.Clear();
		int rewardsCount = quest.GetRewards().Count();
		int i;

		if (rewardsCount== 0)
		{
			RewardPanel.Show(false);
		}
		else if (rewardsCount > 0)
		{
			RewardPanel.Show(true);
			
			if (quest.NeedToSelectReward() && quest.GetRewards().Count() > 1)
			{
				Reward.SetText("#STR_EXPANSION_QUEST_MENU_REWARD_LABEL");
			}
			else
			{
				Reward.SetText("#STR_EXPANSION_QUEST_MENU_REWARDS_LABEL");
			}
			
			for (i = 0; i < quest.GetRewards().Count(); i++)
			{
				ExpansionQuestRewardConfig reward = quest.GetRewards()[i];
				ExpansionQuestMenuItemEntry rewardEntry = new ExpansionQuestMenuItemEntry(reward.GetClassName(), reward.GetAmount());
				rewardEntry.SetQuestRewardConfig(reward);
				rewardEntry.SetQuestMenu(this);
				rewardEntry.SetIsRewardEntry(true);
				m_QuestMenuController.RewardEntries.Insert(rewardEntry);
			}
			
			if (quest.GetHumanityReward() != 0 && GetExpansionSettings().GetHardline().UseHumanity)
			{
				Humanity.Show(true);
				m_QuestMenuController.HumanityVal = quest.GetHumanityReward().ToString();
				m_QuestMenuController.NotifyPropertyChanged("HumanityVal");
			}
			else
			{
				Humanity.Show(false);
			}
		}

		m_QuestMenuController.ObjectiveItems.Clear();

		for (i = 0; i < quest.GetObjectives().Count(); i++)
		{
			ExpansionQuestObjectiveConfigBase objective = quest.GetObjectives()[i];
			int objectiveType = objective.GetObjectiveType();
			QuestDebug(ToString() + "::SetQuest - Objective type: " + objectiveType);
			ExpansionQuestObjectiveTargetConfig objectiveTarget;
			ExpansionQuestObjectiveTravelConfig objectiveTravel;
			ExpansionQuestObjectiveDeliveryConfig objectiveDelivery;
			ExpansionQuestObjectiveCollectionConfig objectiveCollection;

			switch (objectiveType)
			{
				case ExpansionQuestObjectiveType.COLLECT:
				{
					objectiveCollection = ExpansionQuestObjectiveCollectionConfig.Cast(objective);
					string className = objectiveCollection.GetCollection().GetClassName();
					int amount = objectiveCollection.GetCollection().GetAmount();
					ExpansionQuestMenuItemEntry collectObjectiveEntry = new ExpansionQuestMenuItemEntry(className, amount);
					m_QuestMenuController.ObjectiveItems.Insert(collectObjectiveEntry);
					QuestDebug(ToString() + "::SetQuest - Add objective item entry for item: " + className);
					break;
				}
				case ExpansionQuestObjectiveType.DELIVERY:
				{
					objectiveDelivery = ExpansionQuestObjectiveDeliveryConfig.Cast(objective);
					for (int j = 0; j < objectiveDelivery.GetDeliveries().Count(); j++)
					{
						ExpansionQuestObjectiveDelivery delivery = objectiveDelivery.GetDeliveries()[j];
						className = delivery.GetClassName();
						amount = delivery.GetAmount();
						ExpansionQuestMenuItemEntry deliverObjectiveEntry = new ExpansionQuestMenuItemEntry(className, amount);
						m_QuestMenuController.ObjectiveItems.Insert(deliverObjectiveEntry);
						QuestDebug(ToString() + "::SetQuest - Add objective item entry for item: " + className);
					}
				}
			}
		}

		QuestDebug(ToString() + "::SetQuest - End");
	}

	void SetSelectedReward(ExpansionQuestRewardConfig reward)
	{
		m_SelectedReward = reward;
	}
	
	void ResetRewardElements()
	{
		for (int i = 0; i < m_QuestMenuController.RewardEntries.Count(); i++)
		{
			ExpansionQuestMenuItemEntry entry = m_QuestMenuController.RewardEntries[i];
			entry.Reset();
		}
	}
	
	void CloseMenu()
	{
		Hide();
		GetDayZGame().GetExpansionGame().GetExpansionUIManager().CloseMenu();
	}

	void OnCloseButtonClick()
	{
		CloseMenu();
	}

	ref ExpansionQuestConfig GetSelectedQuest()
	{
		return m_SelectedQuest;
	}
	
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (w == Accept)
		{
			AcceptLable.SetColor(ARGB(255, 220, 220, 220));
		}
		else if (w == Complete)
		{
			CompleteLable.SetColor(ARGB(255, 220, 220, 220));
		}
		else if (w == Cancel)
		{
			CancelLable.SetColor(ARGB(255, 220, 220, 220));
		}
		else if (w == Close)
		{
			CloseLable.SetColor(ARGB(255, 220, 220, 220));
		}

		return super.OnMouseEnter(w, x, y);;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (w == Accept)
		{
			AcceptLable.SetColor(ARGB(255, 0, 0, 0));
		}
		else if (w == Complete)
		{
			CompleteLable.SetColor(ARGB(255, 0, 0, 0));
		}
		else if (w == Cancel)
		{
			CancelLable.SetColor(ARGB(255, 0, 0, 0));
		}
		else if (w == Close)
		{
			CloseLable.SetColor(ARGB(255, 0, 0, 0));
		}

		return super.OnMouseLeave(w, enterW, x, y);
	}

	override void OnShow()
	{
		if (!m_QuestModule)
			m_QuestModule =  ExpansionQuestModule.Cast(CF_ModuleCoreManager.Get(ExpansionQuestModule));

		SetFocus(GetLayoutRoot());
	}

	void QuestDebug(string text)
	{
	#ifdef EXPANSIONMODQUESTSUIDEBUG
		Print(text);
	#endif
	}
};

class ExpansionQuestMenuController: ExpansionViewController
{
	string QuestTitle;
	string QuestDescription;
	string QuestObjective;
	string DefaultText;
	string HumanityVal;
	string QuestNPCName;
	ref ObservableCollection<ref ExpansionQuestMenuListEntry> Quests = new ObservableCollection<ref ExpansionQuestMenuListEntry>(this);
	ref ObservableCollection<ref ExpansionQuestMenuItemEntry> RewardEntries = new ObservableCollection<ref ExpansionQuestMenuItemEntry>(this);
	ref ObservableCollection<ref ExpansionQuestMenuItemEntry> ObjectiveItems = new ObservableCollection<ref ExpansionQuestMenuItemEntry>(this);
};