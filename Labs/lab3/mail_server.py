from flask import Flask, jsonify, request

app = Flask(__name__)

messages = []
next_id = 1


@app.post("/messages")
def create_message():
    global next_id

    data = request.get_json(silent=True)

    # TODO 1:
    # Return 400 if the request body is not valid JSON.

    # TODO 2:
    # Require non-empty sender, receiver, and message fields.
    # Return 400 if any required field is missing or empty.

    # TODO 3:
    # Create a new message dictionary with:
    # id, sender, receiver, message
    # Add it to messages, increment next_id,
    # and return the created message with status 201.

    return jsonify({"error": "TODO"}), 501


@app.get("/messages")
def get_messages():
    receiver = request.args.get("receiver")

    # TODO 4:
    # If receiver is not provided, return all messages.
    # If receiver is provided, return only messages for that receiver.

    return jsonify([]), 200


@app.delete("/messages/<int:message_id>")
def delete_message(message_id):
    # TODO 5:
    # Delete the message with the matching id.
    # If found, return {"message": "deleted"} with status 200.
    # Otherwise return {"error": "message not found"} with status 404.

    return jsonify({"error": "TODO"}), 501


if __name__ == "__main__":
    app.run(host="127.0.0.1", port=8000, debug=True)
