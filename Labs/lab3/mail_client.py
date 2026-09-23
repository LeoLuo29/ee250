import argparse
import requests

SERVER = "http://127.0.0.1:8000"


def print_response(response):
    print("Status:", response.status_code)
    print(response.json())


def send_message(sender, receiver, message):
    # TODO 1:
    # POST to /messages with a JSON body containing
    # sender, receiver, and message.
    pass


def list_messages(receiver=None):
    # TODO 2:
    # GET /messages.
    # If receiver is provided, send it as the receiver query parameter.
    pass


def delete_message(message_id):
    # TODO 3:
    # DELETE /messages/<id>.
    pass


def get_parser():
    parser = argparse.ArgumentParser(description="Message REST client")
    subparsers = parser.add_subparsers(dest="command", required=True)

    send_parser = subparsers.add_parser("send")
    send_parser.add_argument("-f", "--from", dest="sender", required=True)
    send_parser.add_argument("-t", "--to", dest="receiver", required=True)
    send_parser.add_argument("-m", "--message", required=True)

    list_parser = subparsers.add_parser("list")
    list_parser.add_argument("--receiver")

    delete_parser = subparsers.add_parser("delete")
    delete_parser.add_argument("message_id", type=int)

    return parser


def main():
    args = get_parser().parse_args()

    if args.command == "send":
        send_message(args.sender, args.receiver, args.message)
    elif args.command == "list":
        list_messages(args.receiver)
    elif args.command == "delete":
        delete_message(args.message_id)


if __name__ == "__main__":
    main()
