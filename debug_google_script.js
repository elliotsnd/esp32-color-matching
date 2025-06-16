// Debug version of Google Apps Script to identify JSON parsing issues
// Deploy this temporarily to diagnose the dulux.json file problem

const DULUX_FILE_ID = '1ICWL4r_LvhUrQAvpObKFzUGDO7DUDaen';

function doGet(e) {
  try {
    // First, let's try to get the file and check its basic properties
    const file = DriveApp.getFileById(DULUX_FILE_ID);
    const content = file.getBlob().getDataAsString();
    
    // Log file information
    console.log('File size:', content.length);
    console.log('File name:', file.getName());
    
    // Check content around position 5106 where the error occurs
    const errorPosition = 5106;
    const contextStart = Math.max(0, errorPosition - 50);
    const contextEnd = Math.min(content.length, errorPosition + 50);
    const context = content.substring(contextStart, contextEnd);
    
    console.log('Content around error position 5106:');
    console.log('Context:', context);
    console.log('Character at position 5106:', content.charAt(errorPosition));
    console.log('Character code:', content.charCodeAt(errorPosition));
    
    // Try to find line 303 (mentioned in error)
    const lines = content.split('\n');
    console.log('Total lines:', lines.length);
    
    if (lines.length >= 303) {
      console.log('Line 303:', lines[302]); // 0-based index
      console.log('Line 302:', lines[301]);
      console.log('Line 304:', lines[303]);
    }
    
    // Try to parse JSON and catch the exact error
    let colors;
    try {
      colors = JSON.parse(content);
      console.log('JSON parsed successfully!');
      console.log('Number of colors:', colors.length);
      
      // Test with a simple color match
      const testR = parseInt(e.parameter.r) || 255;
      const testG = parseInt(e.parameter.g) || 255;
      const testB = parseInt(e.parameter.b) || 255;
      
      // Find first color for testing
      if (colors.length > 0) {
        const firstColor = colors[0];
        return ContentService
          .createTextOutput(JSON.stringify({
            success: true,
            debug: true,
            message: 'JSON parsing successful',
            fileSize: content.length,
            totalColors: colors.length,
            firstColor: firstColor,
            testInput: { r: testR, g: testG, b: testB }
          }))
          .setMimeType(ContentService.MimeType.JSON);
      }
      
    } catch (parseError) {
      console.error('JSON Parse Error:', parseError.message);
      
      // Try to identify the problematic character
      const errorMatch = parseError.message.match(/position (\d+)/);
      if (errorMatch) {
        const pos = parseInt(errorMatch[1]);
        const problemContext = content.substring(Math.max(0, pos - 20), pos + 20);
        
        return ContentService
          .createTextOutput(JSON.stringify({
            success: false,
            debug: true,
            error: 'JSON Parse Error',
            message: parseError.message,
            position: pos,
            context: problemContext,
            characterAtPosition: content.charAt(pos),
            characterCode: content.charCodeAt(pos),
            fileSize: content.length
          }))
          .setMimeType(ContentService.MimeType.JSON);
      }
      
      return ContentService
        .createTextOutput(JSON.stringify({
          success: false,
          debug: true,
          error: 'JSON Parse Error',
          message: parseError.message,
          fileSize: content.length
        }))
        .setMimeType(ContentService.MimeType.JSON);
    }
    
  } catch (error) {
    console.error('General Error:', error);
    return ContentService
      .createTextOutput(JSON.stringify({
        success: false,
        debug: true,
        error: 'General Error',
        message: error.message
      }))
      .setMimeType(ContentService.MimeType.JSON);
  }
}

function doPost(e) {
  // Redirect POST to GET for debugging
  return doGet(e);
}

// Test function to run in the Apps Script editor
function testDebug() {
  const result = doGet({ parameter: { r: 255, g: 255, b: 255 } });
  console.log('Debug result:', result.getContent());
  return result;
}
