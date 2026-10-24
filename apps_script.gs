// Google Apps Script - receives each tap from the ESP32 and adds a row to the sheet.
// Sheet columns: Timestamp | Name | Card ID | Status
// Deploy as a Web app: Execute as "Me", Who has access "Anyone".

function doGet(e) {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getSheets()[0];
  sheet.appendRow([new Date(), e.parameter.name, e.parameter.uid, e.parameter.status]);
  return ContentService.createTextOutput("OK");
}
