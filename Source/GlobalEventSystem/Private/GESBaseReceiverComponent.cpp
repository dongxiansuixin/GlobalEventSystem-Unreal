#include "GESBaseReceiverComponent.h"
#include "GlobalEventSystemBPLibrary.h"

UGESBaseReceiverComponent::UGESBaseReceiverComponent(const FObjectInitializer& init) : UActorComponent(init)
{
	bBindOnBeginPlay = true;
	bUnbindOnEndPlay = true;
	bPinInternalDataForPolling = true;
	bDidReceiveOnEventAtLeastOnce = false;

	BindSettings.ReceivingFunction = TEXT("OnEvent(component)");
}

/*void UGESBaseReceiverComponent::OnEventToPlaceHolderFunction(const FString& PlaceHolderParam)
{
	UE_LOG(LogTemp, Warning, TEXT("PlaceHolder function received event (override BindSettings.ReceivingFunction to match your own receiver)"));
}*/

void UGESBaseReceiverComponent::BeginPlay()
{
	Super::BeginPlay();
	if (bBindOnBeginPlay)
	{
		//special case
		if (BindSettings.ReceivingFunction == TEXT("OnEvent(component)"))
		{
			InternalListener.BindDynamic(this, &UGESBaseReceiverComponent::HandleInternalEvent);
			UGlobalEventSystemBPLibrary::GESBindEventToWildcardDelegate(this, InternalListener, BindSettings.Domain, BindSettings.Event);
		}
		else
		{
			UGlobalEventSystemBPLibrary::GESBindEvent(this, BindSettings.Domain, BindSettings.Event, BindSettings.ReceivingFunction);
		}
	}
}

void UGESBaseReceiverComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bUnbindOnEndPlay)
	{
		if (BindSettings.ReceivingFunction == TEXT("OnEvent(component)"))
		{
			UGlobalEventSystemBPLibrary::GESUnbindWildcardDelegate(this, InternalListener, BindSettings.Domain, BindSettings.Event);
			PinnedData.CleanupPinnedData();
		}
		else
		{
			UGlobalEventSystemBPLibrary::GESUnbindEvent(this, BindSettings.Domain, BindSettings.Event, BindSettings.ReceivingFunction);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UGESBaseReceiverComponent::HandleInternalEvent(const FGESWildcardProperty& WildcardProperty)
{
	LastReceivedProperty = WildcardProperty;

	if (bPinInternalDataForPolling)
	{
		PinnedData.Property = WildcardProperty.Property.Get();
		PinnedData.PropertyPtr = WildcardProperty.PropertyPtr;

		//We need to use pinning to catch non-pinned data emitted
		PinnedData.CopyPropertyToPinnedBuffer();

		LastReceivedProperty.Property = PinnedData.Property;
		LastReceivedProperty.PropertyPtr = PinnedData.PropertyPtr;
	}

	bDidReceiveOnEventAtLeastOnce = true;

	OnEvent.Broadcast(WildcardProperty);
}
