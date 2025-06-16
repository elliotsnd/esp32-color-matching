#!/usr/bin/env python3
"""
Temporary Color Matcher Server
Replaces the broken Google Apps Script for testing ESP32 color matching functionality
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
import json
import math

app = Flask(__name__)
CORS(app)  # Enable CORS for ESP32 requests

# Sample Dulux color database (subset for testing)
DULUX_COLORS = [
    {
        "name": "Vivid White",
        "code": "W01A1",
        "r": 255,
        "g": 255,
        "b": 255,
        "lrv": 90.00
    },
    {
        "name": "First Love Quarter",
        "code": "P08H1Q",
        "r": 255,
        "g": 248,
        "b": 240,
        "lrv": 87.2
    },
    {
        "name": "Pink Tutu Quarter",
        "code": "P04H1Q",
        "r": 255,
        "g": 233,
        "b": 247,
        "lrv": 84.4
    },
    {
        "name": "Cruel Sea",
        "code": "P29A9",
        "r": 33,
        "g": 26,
        "b": 27,
        "lrv": 5.5
    },
    {
        "name": "Decorum",
        "code": "P02D3",
        "r": 179,
        "g": 154,
        "b": 161,
        "lrv": 40.0
    },
    {
        "name": "Deep Leather",
        "code": "P05B9",
        "r": 39,
        "g": 17,
        "b": 0,
        "lrv": 5.9
    }
]

def calculate_color_distance(r1, g1, b1, r2, g2, b2):
    """Calculate Euclidean distance between two RGB colors"""
    return math.sqrt((r1 - r2)**2 + (g1 - g2)**2 + (b1 - b2)**2)

def find_closest_color(target_r, target_g, target_b):
    """Find the closest matching Dulux color"""
    best_match = None
    min_distance = float('inf')
    
    for color in DULUX_COLORS:
        distance = calculate_color_distance(
            target_r, target_g, target_b,
            color['r'], color['g'], color['b']
        )
        
        if distance < min_distance:
            min_distance = distance
            best_match = color
    
    return best_match, min_distance

@app.route('/', methods=['GET'])
def color_match():
    """Handle color matching requests from ESP32"""
    try:
        # Get RGB values from query parameters (ESP32 uses GET with URL params)
        r = int(request.args.get('r', 0))
        g = int(request.args.get('g', 0))
        b = int(request.args.get('b', 0))
        
        print(f"[COLOR MATCH] Received RGB: ({r}, {g}, {b})")
        
        # Validate RGB values
        if not (0 <= r <= 255 and 0 <= g <= 255 and 0 <= b <= 255):
            return jsonify({
                "success": False,
                "error": "Invalid RGB values. Must be 0-255."
            }), 400
        
        # Find closest color match
        match, distance = find_closest_color(r, g, b)
        
        if match:
            response = {
                "success": True,
                "match": {
                    "name": match["name"],
                    "code": match["code"],
                    "r": match["r"],
                    "g": match["g"],
                    "b": match["b"],
                    "lrv": match["lrv"]
                },
                "distance": round(distance, 2),
                "input": {
                    "r": r,
                    "g": g,
                    "b": b
                }
            }
            
            print(f"[COLOR MATCH] Found match: {match['name']} (distance: {distance:.2f})")
            return jsonify(response)
        else:
            return jsonify({
                "success": False,
                "error": "No color match found"
            }), 404
            
    except ValueError as e:
        return jsonify({
            "success": False,
            "error": f"Invalid RGB parameters: {str(e)}"
        }), 400
    except Exception as e:
        print(f"[ERROR] Color matching failed: {str(e)}")
        return jsonify({
            "success": False,
            "error": "Internal server error"
        }), 500

@app.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        "status": "healthy",
        "service": "Color Matcher Server",
        "colors_available": len(DULUX_COLORS)
    })

if __name__ == '__main__':
    print("🎨 Color Matcher Server")
    print("======================")
    print("Temporary replacement for Google Apps Script")
    print("Handles ESP32 color matching requests")
    print("")
    print(f"Available colors: {len(DULUX_COLORS)}")
    for color in DULUX_COLORS:
        print(f"  - {color['name']} ({color['code']}) RGB({color['r']},{color['g']},{color['b']})")
    print("")
    print("Starting server on http://localhost:5000")
    print("To use with ESP32, update GOOGLE_SCRIPT_URL to: http://YOUR_IP:5000")
    print("")
    
    app.run(host='0.0.0.0', port=5000, debug=True)
