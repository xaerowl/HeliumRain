#pragma once

#include "FlareQuestManager.h"
#include "FlareQuestCondition.generated.h"

class UFlareQuest;
class AFlarePlayerController;
class AFlareGame;
class UFlareCompany;
class UFlareSimulatedSector;
class UFlareSimulatedSpacecraft;
struct FFlarePlayerObjectiveData;
struct FFlareBundle;
struct FFlareResourceDescription;

/** A quest Step condition */
UCLASS(abstract)
class HELIUMRAIN_API UFlareQuestCondition: public UObject
{
	GENERATED_UCLASS_BODY()

public:
	virtual void Restore(const FFlareBundle* Bundle)
	{
	}

	virtual void Save(FFlareBundle* Bundle)
	{
	}

	virtual void AddSave(TArray<FFlareQuestConditionSave>& Data);

	virtual void OnTradeDone(UFlareSimulatedSpacecraft* SourceSpacecraft, UFlareSimulatedSpacecraft* DestinationSpacecraft, FFlareResourceDescription* Resource, int32 Quantity) {}

	void SetConditionIndex(int32 Index)
	{
		ConditionIndex = Index;
	}

protected:

	  void LoadInternal(UFlareQuest* ParentQuest, FName ConditionIdentifier = NAME_None)
	  {
		  Identifier = ConditionIdentifier;
		  Quest = ParentQuest;
	  }

	/*----------------------------------------------------
		Protected data
	----------------------------------------------------*/
	int32                                   ConditionIndex;
	FText                       InitialLabel;
	FText						TerminalLabel;
	FName                       Identifier;
	TArray<EFlareQuestCallback::Type> Callbacks;

	UFlareQuest* Quest;
public:

	/*----------------------------------------------------
	 Getters
	----------------------------------------------------*/

	virtual bool IsCompleted();

	int32 GetConditionIndex()
	{
		return ConditionIndex;
	}

	virtual FText GetInitialLabel()
	{
		return InitialLabel;
	}

	FText GetTerminalLabel()
	{
		return TerminalLabel;
	}

	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

	virtual TArray<EFlareQuestCallback::Type>& GetConditionCallbacks()
	{
		return Callbacks;
	}

	static bool CheckConditions(TArray<UFlareQuestCondition*>& Conditions, bool EmptyResult);

	static bool CheckFailConditions(TArray<UFlareQuestCondition*>& Conditions, bool EmptyResult);

	static void AddConditionCallbacks(TArray<EFlareQuestCallback::Type>& Callbacks, const TArray<UFlareQuestCondition*>& Conditions);

	static const FFlareBundle* GetStepConditionBundle(UFlareQuestCondition* Condition, const TArray<FFlareQuestConditionSave>& Data);

	FName GetIdentifier()
	{
		return Identifier;
	}

	AFlareGame* GetGame();
	AFlarePlayerController* GetPC();

};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionFlyingShipClass: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()


public:

	static UFlareQuestConditionFlyingShipClass* Create(UFlareQuest* ParentQuest, FName ShipClassParam);
	void Load(UFlareQuest* ParentQuest, FName ShipClassParam);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

protected:

	FName ShipClass;
};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionSectorActive: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()


public:

	static UFlareQuestConditionSectorActive* Create(UFlareQuest* ParentQuest, UFlareSimulatedSector* SectorParam);
	void Load(UFlareQuest* ParentQuest, UFlareSimulatedSector* SectorParam);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

protected:

	UFlareSimulatedSector* Sector;
};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionSectorVisited: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()


public:

	static UFlareQuestConditionSectorVisited* Create(UFlareQuest* ParentQuest, UFlareSimulatedSector* SectorParam);
	void Load(UFlareQuest* ParentQuest, UFlareSimulatedSector* SectorParam);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

protected:

	UFlareSimulatedSector* Sector;
};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionMinCollinearVelocity: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()


public:

	static UFlareQuestConditionMinCollinearVelocity* Create(UFlareQuest* ParentQuest, FName ConditionIdentifier, float VelocityLimitParam);
	void Load(UFlareQuest* ParentQuest, FName ConditionIdentifier, float VelocityLimitParam);
	virtual void Restore(const FFlareBundle* Bundle);
	virtual void Save(FFlareBundle* Bundle);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

	float GetCollinearVelocity();


protected:

	float VelocityLimit;
	bool HasInitialVelocity;
	float InitialVelocity;
};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionMaxCollinearVelocity: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()


public:

	static UFlareQuestConditionMaxCollinearVelocity* Create(UFlareQuest* ParentQuest, FName ConditionIdentifier, float VelocityLimitParam);
	void Load(UFlareQuest* ParentQuest, FName ConditionIdentifier, float VelocityLimitParam);
	virtual void Restore(const FFlareBundle* Bundle);
	virtual void Save(FFlareBundle* Bundle);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

	float GetCollinearVelocity();


protected:

	float VelocityLimit;
	bool HasInitialVelocity;
	float InitialVelocity;
};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionMinCollinear: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()


public:

	static UFlareQuestConditionMinCollinear* Create(UFlareQuest* ParentQuest, float CollinearLimitParam);
	void Load(UFlareQuest* ParentQuest, float CollinearLimitParam);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

	float GetCollinear();

protected:

	float CollinearLimit;
};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionMaxCollinear: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()


public:

	static UFlareQuestConditionMaxCollinear* Create(UFlareQuest* ParentQuest, float CollinearLimitParam);
	void Load(UFlareQuest* ParentQuest, float CollinearLimitParam);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

	float GetCollinear();

protected:

	float CollinearLimit;
};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionMinRotationVelocity: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()


public:

	static UFlareQuestConditionMinRotationVelocity* Create(UFlareQuest* ParentQuest, FVector LocalAxisParam, float AngularVelocityLimitParam);
	void Load(UFlareQuest* ParentQuest, FVector LocalAxisParam, float AngularVelocityLimitParam);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

	float GetVelocityInAxis();

protected:

	FVector LocalAxis;
	float AngularVelocityLimit;
};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionMaxRotationVelocity: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()


public:

	static UFlareQuestConditionMaxRotationVelocity* Create(UFlareQuest* ParentQuest, FVector LocalAxisParam, float AngularVelocityLimitParam);
	void Load(UFlareQuest* ParentQuest, FVector LocalAxisParam, float AngularVelocityLimitParam);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

	float GetVelocityInAxis();

protected:

	FVector LocalAxis;
	float AngularVelocityLimit;
};


//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionShipAlive: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()


public:

	static UFlareQuestConditionShipAlive* Create(UFlareQuest* ParentQuest, FName ShipIdentifierParam);
	void Load(UFlareQuest* ParentQuest, FName ShipIdentifierParam);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

	bool IsShipAlive();

protected:

	FName ShipIdentifier;
};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionQuestSuccessful: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()


public:

	static UFlareQuestConditionQuestSuccessful* Create(UFlareQuest* ParentQuest, FName QuestParam);
	void Load(UFlareQuest* ParentQuest, FName QuestParam);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

protected:

	FName TargetQuest;
};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionQuestFailed: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()


public:

	static UFlareQuestConditionQuestFailed* Create(UFlareQuest* ParentQuest, FName QuestParam);
	void Load(UFlareQuest* ParentQuest, FName QuestParam);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

protected:

	FName TargetQuest;
};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionFollowRelativeWaypoints: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()


public:

	static UFlareQuestConditionFollowRelativeWaypoints* Create(UFlareQuest* ParentQuest, FName ConditionIdentifier, TArray<FVector> VectorListParam);
	void Load(UFlareQuest* ParentQuest, FName ConditionIdentifier, TArray<FVector> VectorListParam);
	virtual void Restore(const FFlareBundle* Bundle);
	virtual void Save(FFlareBundle* Bundle);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

protected:

	void Init();

	TArray<FVector> VectorList;

	bool IsInit;
	int32 CurrentProgression;
	FTransform InitialTransform;
};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionFollowRandomWaypoints: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()


public:

	static UFlareQuestConditionFollowRandomWaypoints* Create(UFlareQuest* ParentQuest, FName ConditionIdentifier);
	void Load(UFlareQuest* ParentQuest, FName ConditionIdentifier);
	virtual void Restore(const FFlareBundle* Bundle);
	virtual void Save(FFlareBundle* Bundle);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

protected:

	void Init();

	void GenerateWaypointSegments(TArray<FVector>& WaypointList, float& Distance, float MaxDistance, float StepDistance,
								  FVector& BaseDirection, FVector& BaseLocation, FVector TargetLocation,
								  float TargetMaxTurnDistance);



	bool IsInit;
	TArray<FVector> Waypoints;
	int32 CurrentProgression;
};


//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionDockAt: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()

public:

	static UFlareQuestConditionDockAt* Create(UFlareQuest* ParentQuest, UFlareSimulatedSpacecraft* Station);
	void Load(UFlareQuest* ParentQuest, UFlareSimulatedSpacecraft* Station);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);
	virtual FText GetInitialLabel();

	FName TargetShipMatchId;
	FName TargetShipSaveId;

protected:

	UFlareSimulatedSpacecraft* TargetStation;
	bool Completed;
};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionAtWar: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()


public:

	static UFlareQuestConditionAtWar* Create(UFlareQuest* ParentQuest, UFlareCompany* Company);
	void Load(UFlareQuest* ParentQuest, UFlareCompany* Company);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

protected:

	UFlareCompany* TargetCompany;
};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionSpacecraftNoMoreExist: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()

public:
	static UFlareQuestConditionSpacecraftNoMoreExist* Create(UFlareQuest* ParentQuest, UFlareSimulatedSpacecraft* TargetSpacecraftParam, FName TargetSpacecraftIdParam = NAME_None);
	void Load(UFlareQuest* ParentQuest, UFlareSimulatedSpacecraft* TargetSpacecraftParam, FName TargetSpacecraftIdParam);

	virtual bool IsCompleted();
	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);

protected:

	UFlareSimulatedSpacecraft* TargetSpacecraft;
	FName TargetSpacecraftId;
};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionBuyAtStation: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()

public:

	static UFlareQuestConditionBuyAtStation* Create(UFlareQuest* ParentQuest, FName ConditionIdentifierParam, UFlareSimulatedSpacecraft* StationParam, FFlareResourceDescription* ResourceParam, int32 QuantityParam);
	void Load(UFlareQuest* ParentQuest, FName ConditionIdentifierParam, UFlareSimulatedSpacecraft* StationParam, FFlareResourceDescription* ResourceParam, int32 QuantityParam);

	virtual bool IsCompleted();
	virtual void Restore(const FFlareBundle* Bundle);
	virtual void Save(FFlareBundle* Bundle);

	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);
	virtual void OnTradeDone(UFlareSimulatedSpacecraft* SourceSpacecraft, UFlareSimulatedSpacecraft* DestinationSpacecraft, FFlareResourceDescription* TradeResource, int32 TradeQuantity);

protected:

	UFlareSimulatedSpacecraft* TargetStation;
	FFlareResourceDescription* Resource;
	int32 Quantity;
	int32 CurrentProgression;
};

//////////////////////////////////////////////////////
UCLASS()
class HELIUMRAIN_API UFlareQuestConditionSellAtStation: public UFlareQuestCondition
{
	GENERATED_UCLASS_BODY()

public:

	static UFlareQuestConditionSellAtStation* Create(UFlareQuest* ParentQuest, FName ConditionIdentifierParam, UFlareSimulatedSpacecraft* StationParam, FFlareResourceDescription* ResourceParam, int32 QuantityParam);
	void Load(UFlareQuest* ParentQuest, FName ConditionIdentifierParam, UFlareSimulatedSpacecraft* StationParam, FFlareResourceDescription* ResourceParam, int32 QuantityParam);

	virtual bool IsCompleted();
	virtual void Restore(const FFlareBundle* Bundle);
	virtual void Save(FFlareBundle* Bundle);

	virtual void AddConditionObjectives(FFlarePlayerObjectiveData* ObjectiveData);
	virtual void OnTradeDone(UFlareSimulatedSpacecraft* SourceSpacecraft, UFlareSimulatedSpacecraft* DestinationSpacecraft, FFlareResourceDescription* TradeResource, int32 TradeQuantity);

protected:

	UFlareSimulatedSpacecraft* TargetStation;
	FFlareResourceDescription* Resource;
	int32 Quantity;
	int32 CurrentProgression;
};