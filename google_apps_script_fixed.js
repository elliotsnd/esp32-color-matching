// Google Apps Script for Dulux Color Matching
// Deploy this as a web app and use the URL in your ESP32

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
  const testR = e.parameter.r || 245;
  const testG = e.parameter.g || 227;
  const testB = e.parameter.b || 227;
  
  const match = findClosestColorMatch(parseInt(testR), parseInt(testG), parseInt(testB));
  
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
    const content = file.getBlob().getDataAsString();
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
      
      // Check for exact matches first (distance = 0)
      if (distance === 0) {
        console.log(`Exact match found: ${color.name} - Distance: ${distance}`);
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

      // For very close matches (distance <= 1.5), prioritize by LRV similarity for whites
      if (distance <= 1.5) {
        // Check if this is a white color (high RGB values)
        const isWhiteColor = colorR >= 240 && colorG >= 240 && colorB >= 240;
        const isInputWhite = inputR >= 240 && inputG >= 240 && inputB >= 240;

        if (isWhiteColor && isInputWhite) {
          // For white colors, also consider LRV (Light Reflectance Value) similarity
          const avgInputRGB = (inputR + inputG + inputB) / 3;
          const avgColorRGB = (colorR + colorG + colorB) / 3;
          const rgbDifference = Math.abs(avgInputRGB - avgColorRGB);

          // Weighted score: 70% distance + 30% RGB average difference
          const weightedScore = (distance * 0.7) + (rgbDifference * 0.3);

          if (weightedScore < smallestDistance) {
            smallestDistance = weightedScore;
            closestMatch = {
              name: color.name,
              code: color.code,
              lrv: color.lrv,
              id: color.id,
              lightText: color.lightText,
              r: color.r,
              g: color.g,
              b: color.b,
              distance: distance.toFixed(2),
              weightedScore: weightedScore.toFixed(2)
            };
          }
          continue; // Skip the regular distance check for white colors
        }
      }
      
      // Track the closest match overall (for non-white colors or when white logic didn't apply)
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
  // Test with the first color from your example
  const result = findClosestColorMatch(245, 222, 227);
  console.log('Test result:', result);
  return result;
}
