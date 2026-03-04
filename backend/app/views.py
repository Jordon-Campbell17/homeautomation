# app/routes.py (fixed version)

from app import app, Config, mongo, Mqtt
from flask import request, jsonify
from json import dumps
from time import time
from os.path import join, exists
from flask import send_from_directory, getcwd

#####################################
#   Routing for your application    #
#####################################

# 1. CREATE ROUTE FOR '/api/set/combination'
@app.route('/api/set/combination', methods=['POST'])
def set_combination():
    passcode = request.form.get('passcode')
    if passcode and passcode.isdigit() and len(passcode) == 4:
        result = mongo.setPasscode(passcode)
        if result:
            return jsonify({"status": "complete", "data": "complete"})
    return jsonify({"status": "failed", "data": "failed"})

# 2. CREATE ROUTE FOR '/api/check/combination'
@app.route('/api/check/combination', methods=['POST'])
def check_combination():
    passcode = request.form.get('passcode')
    if passcode:
        result = mongo.checkPasscode(passcode)
        if result > 0:
            return jsonify({"status": "complete", "data": "complete"})
    return jsonify({"status": "failed", "data": "failed"})

# 3. CREATE ROUTE FOR '/api/update'
@app.route('/api/update', methods=['POST'])
def update():
    try:
        data = request.get_json()
        if not data:
            return jsonify({"status": "failed", "reason": "No JSON received"})

        # Ensure 'id' exists
        if 'id' not in data:
            return jsonify({"status": "failed", "reason": "'id' missing in JSON"})

        # Add timestamp
        data['timestamp'] = int(time())

        # Publish via MQTT (optional)
        try:
            Mqtt.publish(str(data['id']), dumps(data))
        except Exception as e:
            print("MQTT publish error:", e)

        # Insert into MongoDB
        try:
            result = mongo.addRadar(data)  # your existing mongo wrapper
            print("Mongo insert result:", result)
            return jsonify({"status": "complete", "data": "complete"})
        except Exception as e:
            print("Mongo insertion error:", e)
            return jsonify({"status": "failed", "reason": "Mongo insertion error"})

    except Exception as e:
        print("Update route error:", e)
        return jsonify({"status": "failed", "data": "failed"})

# 4. GET reserve in range
@app.route('/api/reserve/<start>/<end>', methods=['GET'])
def get_reserve(start, end):
    result = mongo.getRadarInRange(start, end)
    if result:
        return jsonify({"status": "found", "data": result})
    return jsonify({"status": "failed", "data": 0})

# 5. GET average in range
@app.route('/api/avg/<start>/<end>', methods=['GET'])
def get_avg(start, end):
    result = mongo.avgReserve(start, end)
    if result:
        return jsonify({"status": "found", "data": result[0]['average']})
    return jsonify({"status": "failed", "data": 0})

# 6. GET file from uploads
@app.route('/api/file/get/<filename>', methods=['GET'])
def get_images(filename):
    directory = join(getcwd(), Config.UPLOADS_FOLDER)
    filePath = join(directory, filename)
    if exists(filePath):
        return send_from_directory(directory, filename)
    return jsonify({"status": "file not found"}), 404

#####################################
#       After request headers       #
#####################################
@app.after_request
def add_header(response):
    response.headers['X-UA-Compatible'] = 'IE=Edge,chrome=1'
    response.headers['Cache-Control'] = 'public, max-age=0'
    return response

# Custom 405 handler
@app.errorhandler(405)
def page_not_found(error):
    return jsonify({"status": 405}), 405