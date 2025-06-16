// Simple test version - deploy this temporarily to verify the file ID works
const DULUX_FILE_ID = '1ICWL4r_LvhUrQAvpObKFzUGDO7DUDaen';

function doGet(e) {
  try {
    // Just try to read the file and return basic info
    const file = DriveApp.getFileById(DULUX_FILE_ID);
    const content = file.getBlob().getDataAsString();
    
    // Try to parse JSON
    const colors = JSON.parse(content);
    
    // Return success with basic info
    return ContentService
      .createTextOutput(JSON.stringify({
        success: true,
        message: "File loaded successfully",
        fileSize: content.length,
        totalColors: colors.length,
        firstColor: colors[0],
        testInput: {
          r: e.parameter.r || 255,
          g: e.parameter.g || 255,
          b: e.parameter.b || 255
        }
      }))
      .setMimeType(ContentService.MimeType.JSON);
      
  } catch (error) {
    return ContentService
      .createTextOutput(JSON.stringify({
        success: false,
        error: error.message,
        fileId: DULUX_FILE_ID
      }))
      .setMimeType(ContentService.MimeType.JSON);
  }
}

function doPost(e) {
  return doGet(e);
}
