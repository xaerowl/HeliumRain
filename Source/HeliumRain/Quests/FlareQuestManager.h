#pragma once

#include "FlareQuestManager.generated.h"

class UFlareQuest;
class AFlareGame;
class AFlareSpacecraft;
class UFlareQuestGenerator;
struct FFlareQuestDescription;
class UFlareSimulatedSpacecraft;

/** Quest action type */
UENUM()
namespace EFlareQuestStatus
{
	enum Type
	{
		PENDING, // Wait available condition // ACTIVE
		AVAILABLE, // Wait acceptation       // ACTIVE  // VISIBLE
		ONGOING, // In progress               // ACTIVE  // VISIBLE
		SUCCESSFUL,
		ABANDONED,
		FAILED
	};
}

/** Quest callback type */
UENUM()
namespace EFlareQuestCallback
{
	enum Type
	{
		TICK_FLYING, // Trig the quest at each tick
		SECTOR_VISITED, // Trig when a sector is visited
		SECTOR_ACTIVE, // Trig when a sector is activated
		FLY_SHIP, // Trig the quest when a ship is flyed
		QUEST_CHANGED, // Trig when a quest status change
		SHIP_DOCKED, // Trig a ship is docked
		WAR_STATE_CHANGED, // Trig when a war state changed
		SPACECRAFT_DESTROYED, // Trig when a spacecraft is destroyed
		TRADE_DONE, // Trig when a trade is done
		NEXT_DAY, // Trig after a simulate
		SPACECRAFT_CAPTURED, // Trig when a spacecraft is captured
		TRAVEL_STARTED, // Trig when a fleet start a travel
	};
}

/** Quest current step status save data */
USTRUCT()
struct FFlareQuestConditionSave
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, Category = Save)
	FName ConditionIdentifier;

	FFlareBundle Data;
};

/** Quest progress status save data */
USTRUCT()
struct FFlareQuestProgressSave
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, Category = Save)
	FName QuestIdentifier;

	UPROPERTY(VisibleAnywhere, Category = Save)
	TEnumAsByte<EFlareQuestStatus::Type> Status;

	UPROPERTY(EditAnywhere, Category = Save)
	int64 AvailableDate;

	UPROPERTY(EditAnywhere, Category = Save)
	int64 AcceptationDate;

	UPROPERTY(VisibleAnywhere, Category = Save)
	TArray<FName> SuccessfullSteps;

	UPROPERTY(VisibleAnywhere, Category = Save)
	TArray<FFlareQuestConditionSave> CurrentStepProgress;

	UPROPERTY(VisibleAnywhere, Category = Save)
	TArray<FFlareQuestConditionSave> TriggerConditionsSave;

	UPROPERTY(VisibleAnywhere, Category = Save)
	TArray<FFlareQuestConditionSave> ExpirationConditionsSave;

	UPROPERTY(VisibleAnywhere, Category = Save)
	FFlareBundle Data;
};

/** Quest generated quest save */
USTRUCT()
struct FFlareGeneratedQuestSave
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, Category = Save)
	FName QuestClass;
	FFlareBundle Data;
};

/** Quest status save data */
USTRUCT()
struct FFlareQuestSave
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, Category = Save)
	FName SelectedQuest;

	UPROPERTY(VisibleAnywhere, Category = Save)
	TArray<FFlareQuestProgressSave> QuestProgresses;

	UPROPERTY(VisibleAnywhere, Category = Save)
	TArray<FName> SuccessfulQuests;

	UPROPERTY(VisibleAnywhere, Category = Save)
	TArray<FName> AbandonedQuests;

	UPROPERTY(VisibleAnywhere, Category = Save)
	TArray<FName> FailedQuests;

	UPROPERTY(VisibleAnywhere, Category = Save)
	bool PlayTutorial;

	UPROPERTY(VisibleAnywhere, Category = Save)
	TArray<FFlareGeneratedQuestSave> GeneratedQuests;

	UPROPERTY(VisibleAnywhere, Category = Save)
	int64 NextGeneratedQuestIndex;
};


/** Quest system manager */
UCLASS()
class HELIUMRAIN_API UFlareQuestManager: public UObject
{
	GENERATED_UCLASS_BODY()

public:
   /*----------------------------------------------------
	   Save
   ----------------------------------------------------*/

	/** Load the quests status from a save file */
	virtual void Load(const FFlareQuestSave& Data);

	/** Save the quests status to a save file */
	virtual FFlareQuestSave* Save();


	/*----------------------------------------------------
		Quest management
	----------------------------------------------------*/

	/** Accept this quest */
	void AcceptQuest(UFlareQuest* Quest);

	/** Abandon this quest */
	void AbandonQuest(UFlareQuest* Quest);
		
	/** Select this quest */
	void SelectQuest(UFlareQuest* Quest);

	/** Auto select a quest */
	void AutoSelectQuest();

	void LoadBuildinQuest();

	void LoadCatalogQuests();

	void LoadDynamicQuests();

	void AddQuest(UFlareQuest* Quest);

   /*----------------------------------------------------
	   Callback
   ----------------------------------------------------*/

	virtual void LoadCallbacks(UFlareQuest* Quest);

	virtual void ClearCallbacks(UFlareQuest* Quest);

	void OnCallbackEvent(EFlareQuestCallback::Type EventType);

	virtual void OnFlyShip(AFlareSpacecraft* Ship);

	virtual void OnSectorActivation(UFlareSimulatedSector* Sector);

	virtual void OnSectorVisited(UFlareSimulatedSector* Sector);

	virtual void OnShipDocked(UFlareSimulatedSpacecraft* Station, UFlareSimulatedSpacecraft* Ship);

	virtual void OnWarStateChanged(UFlareCompany* Company1, UFlareCompany* Company2);

	virtual void OnNextDay();

	virtual void OnTick(float DeltaSeconds);

	virtual void OnTravelStarted(UFlareTravel* Travel);

	virtual void OnTravelEnded(UFlareFleet* Fleet);

	virtual void OnQuestStatusChanged(UFlareQuest* Quest);

	virtual void OnQuestSuccess(UFlareQuest* Quest);

	virtual void OnQuestFail(UFlareQuest* Quest, bool Notify);

	virtual void OnQuestOngoing(UFlareQuest* Quest);

	virtual void OnQuestAvailable(UFlareQuest* Quest);

	virtual void OnSpacecraftDestroyed(UFlareSimulatedSpacecraft* Spacecraft, bool Uncontrollable, UFlareCompany* Source);

	virtual void OnTradeDone(UFlareSimulatedSpacecraft* SourceSpacecraft, UFlareSimulatedSpacecraft* DestinationSpacecraft, FFlareResourceDescription* Resource, int32 Quantity);

	virtual void OnSpacecraftCaptured(UFlareSimulatedSpacecraft* CapturedSpacecraftBefore, UFlareSimulatedSpacecraft* CapturedSpacecraftAfter);

protected:

   /*----------------------------------------------------
	   Protected data
   ----------------------------------------------------*/

	UPROPERTY()
	TArray<UFlareQuest*>	                 PendingQuests;

	UPROPERTY()
	TArray<UFlareQuest*>	                 AvailableQuests;

	UPROPERTY()
	TArray<UFlareQuest*>	                 OngoingQuests;

	UPROPERTY()
	TArray<UFlareQuest*>	                 OldQuests;

	UPROPERTY()
	TArray<UFlareQuest*>	                 Quests;
	
	UFlareQuest*			                 SelectedQuest;

	TMap<EFlareQuestCallback::Type, TArray<UFlareQuest*>> CallbacksMap;

	FFlareQuestSave			                 QuestData;

	AFlareGame*                              Game;

	TArray<FName>							 ActiveQuestIdentifiers;

	UPROPERTY()
	UFlareQuestGenerator*					 QuestGenerator;

public:

	/*----------------------------------------------------
		Getters
	----------------------------------------------------*/

	inline AFlareGame* GetGame() const
	{
		return Game;
	}

	inline UFlareQuest* GetSelectedQuest()
	{
		return SelectedQuest;
	}

	inline TArray<UFlareQuest*>& GetAvailableQuests()
	{
		return AvailableQuests;
	}

	inline TArray<UFlareQuest*>& GetOngoingQuests()
	{
		return OngoingQuests;
	}

	inline TArray<UFlareQuest*>& GetPreviousQuests()
	{
		return OldQuests;
	}

	bool IsQuestOngoing(UFlareQuest* Quest);

	bool IsOldQuest(UFlareQuest* Quest);

	bool IsQuestAvailable(UFlareQuest* Quest);

	bool IsQuestSuccessfull(UFlareQuest* Quest);

	bool IsQuestFailed(UFlareQuest* Quest);

	UFlareQuest* FindQuest(FName QuestIdentifier);

	int32 GetVisibleQuestCount();

	int32 GetVisibleQuestCount(UFlareCompany* Client);

	UFlareQuestGenerator* GetQuestGenerator()
	{
		return QuestGenerator;
	}
};
