function test_contractRename()
{
	newProject();
	tryCompare(mainApplication.mainContent.projectNavigator.sections.itemAt(0).model.get(0), "name", "Contract");
	editContract("contract Renamed {}");
	mainApplication.mainContent.startQuickDebugging();
	if (!ts.waitForSignal(mainApplication.clientModel, "runComplete()", 5000))
		fail("Error running transaction");
	wait(1000);
	tryCompare(mainApplication.mainContent.projectNavigator.sections.itemAt(0).model.get(0), "name", "Renamed");
	mainApplication.projectModel.stateListModel.editState(0);
	mainApplication.projectModel.stateDialog.model.editTransaction(2);
	var transactionDialog = mainApplication.projectModel.stateDialog.transactionDialog;
	tryCompare(transactionDialog, "contractId", "Renamed");
	tryCompare(transactionDialog, "functionId", "Renamed");
	transactionDialog.close();
	mainApplication.projectModel.stateDialog.close();
	tryCompare(mainApplication.mainContent.rightPane.transactionLog.transactionModel.get(2), "contract", "Renamed");
}
