// Robust Google Apps Script for Dulux Color Matching with JSON error handling
const DULUX_FILE_ID = '1ICWL4r_LvhUrQAvpObKFzUGDO7DUDaen';

function doPost(e) {
  try {
    const params = JSON.parse(e.postData.contents);
    const inputR = parseInt(params.r);
    const inputG = parseInt(params.g);
    const inputB = parseInt(params.b);
    
    if (isNaN(inputR) || isNaN(inputG) || isNaN(inputB)) {
      return createErrorResponse('Invalid RGB values provided');
    }
    
    console.log(`Searching for color match: R=${inputR}, G=${inputG}, B=${inputB}`);
    
    const match = findClosestColorMatch(inputR, inputG, inputB);
    
    if (match) {
      console.log(`Found match: ${match.name} (${match.code})`);
      return createSuccessResponse(match);
    } else {
      return createErrorResponse('No color match found');
    }
    
  } catch (error) {
    console.error('Error in doPost:', error);
    return createErrorResponse('Internal server error: ' + error.message);
  }
}

function doGet(e) {
  // For testing purposes and ESP32 GET requests
  const testR = parseInt(e.parameter.r) || 255;
  const testG = parseInt(e.parameter.g) || 255;
  const testB = parseInt(e.parameter.b) || 255;
  
  const match = findClosestColorMatch(testR, testG, testB);
  
  if (match) {
    return createSuccessResponse(match);
  } else {
    return createErrorResponse('No color match found');
  }
}

function findClosestColorMatch(inputR, inputG, inputB) {
  try {
    // Fetch the dulux.json file from Google Drive
    const file = DriveApp.getFileById(DULUX_FILE_ID);
    let content = file.getBlob().getDataAsString();
    
    // Clean up potential JSON formatting issues
    content = content.trim();
    
    // Remove any BOM (Byte Order Mark) characters
    if (content.charCodeAt(0) === 0xFEFF) {
      content = content.slice(1);
    }
    
    // Try to fix common JSON issues
    // Remove any trailing commas before closing brackets/braces
    content = content.replace(/,(\s*[}\]])/g, '$1');
    
    // Parse the JSON
    const colors = JSON.parse(content);
    
    let closestMatch = null;
    let smallestDistance = Infinity;
    
    // Search through all colors
    for (let i = 0; i < colors.length; i++) {
      const color = colors[i];
      const colorR = parseInt(color.r);
      const colorG = parseInt(color.g);
      const colorB = parseInt(color.b);
      
      // Calculate Euclidean distance in RGB space
      const distance = Math.sqrt(
        Math.pow(inputR - colorR, 2) + 
        Math.pow(inputG - colorG, 2) + 
        Math.pow(inputB - colorB, 2)
      );
      
      // Check if this is within tolerance (±1 for each RGB component)
      const rDiff = Math.abs(inputR - colorR);
      const gDiff = Math.abs(inputG - colorG);
      const bDiff = Math.abs(inputB - colorB);
      
      // If within ±1 tolerance for all components, it's a perfect match
      if (rDiff <= 1 && gDiff <= 1 && bDiff <= 1) {
        console.log(`Perfect match found: ${color.name} - Distance: ${distance}`);
        return {
          name: color.name,
          code: color.code,
          lrv: color.lrv,
          id: color.id,
          lightText: color.lightText,
          r: color.r,
          g: color.g,
          b: color.b,
          distance: distance.toFixed(2)
        };
      }
      
      // Track the closest match overall
      if (distance < smallestDistance) {
        smallestDistance = distance;
        closestMatch = {
          name: color.name,
          code: color.code,
          lrv: color.lrv,
          id: color.id,
          lightText: color.lightText,
          r: color.r,
          g: color.g,
          b: color.b,
          distance: distance.toFixed(2)
        };
      }
    }
    
    console.log(`Closest match: ${closestMatch ? closestMatch.name : 'None'} - Distance: ${smallestDistance}`);
    return closestMatch;
    
  } catch (error) {
    console.error('Error in findClosestColorMatch:', error);
    
    // If JSON parsing fails, try to provide more specific error info
    if (error.message.includes('position')) {
      const positionMatch = error.message.match(/position (\d+)/);
      if (positionMatch) {
        const pos = parseInt(positionMatch[1]);
        console.error(`JSON error at position ${pos}`);
      }
    }
    
    throw error;
  }
}

function createSuccessResponse(match) {
  const response = {
    success: true,
    match: match
  };
  
  return ContentService
    .createTextOutput(JSON.stringify(response))
    .setMimeType(ContentService.MimeType.JSON);
}

function createErrorResponse(message) {
  const response = {
    success: false,
    error: message
  };
  
  return ContentService
    .createTextOutput(JSON.stringify(response))
    .setMimeType(ContentService.MimeType.JSON);
}

// Test function - you can run this in the Apps Script editor to test
function testColorMatching() {
  // Test with Vivid White
  const result = findClosestColorMatch(255, 255, 255);
  console.log('Test result:', result);
  return result;
}
