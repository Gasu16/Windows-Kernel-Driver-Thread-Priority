#include <ntifs.h>
#include <ntddk.h>
#include "MyDriverCommon.h"
#include <sal.h>
#include <bcrypt.h>

// prototypes

void MyDriverUnload(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS MyDriverCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS MyDriverDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

// DriverEntry

extern "C" NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	KdPrint(("MyDriver DriverEntry started\n"));

	DriverObject->DriverUnload = MyDriverUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = MyDriverCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = MyDriverCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyDriverDeviceControl;

	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\MyDriver");
	//RtlInitUnicodeString(&devName, L"\\Device\\ThreadBoost");
	PDEVICE_OBJECT DeviceObject;
	NTSTATUS status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create device (0x%08X)\n", status));
		return status;
	}

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\MyDriver");
	status = IoCreateSymbolicLink(&symLink, &devName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
		IoDeleteDevice(DeviceObject);
		return status;
	}

	KdPrint(("MyDriver DriverEntry completed successfully\n"));

	return STATUS_SUCCESS;
}

void MyDriverUnload(_In_ PDRIVER_OBJECT DriverObject) {
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\MyDriver");
	// delete symbolic link
	IoDeleteSymbolicLink(&symLink);

	// delete device object
	IoDeleteDevice(DriverObject->DeviceObject);

	KdPrint(("MyDriver unloaded\n"));
}

_Use_decl_annotations_
NTSTATUS MyDriverCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS MyDriverDeviceControl(PDEVICE_OBJECT, PIRP Irp) {
	// get our IO_STACK_LOCATION
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto status = STATUS_SUCCESS;

	switch (stack->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_MY_DRIVER_SET_PRIORITY:
	{
		// do the work
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ThreadData)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		auto data = (ThreadData*)stack->Parameters.DeviceIoControl.Type3InputBuffer;
		if (data == nullptr) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		if (data->Priority < 1 || data->Priority > 31) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		PETHREAD Thread;
		status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &Thread);
		if (!NT_SUCCESS(status))
			break;

		KeSetPriorityThread((PKTHREAD)Thread, data->Priority);
		ObDereferenceObject(Thread);
		KdPrint(("Thread Priority change for %d to %d succeeded!\n",
			data->ThreadId, data->Priority));
		break;
	}

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}