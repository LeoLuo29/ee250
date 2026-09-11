# EE 250L Fall 2026 - Lab 2 Responses

**Name:**  
**USC username:**  

Answer the questions in your own words. Add screenshots to the PDF submission as described in the lab handout.

## Part 1: Evaluating TCP and UDP

### 1. UDP under packet loss
How did introducing 50% packet loss affect the UDP messages you observed? Explain why.

**Answer:**

### 2. TCP under packet loss
How did introducing 50% packet loss affect the TCP exchange? Did the application data disappear, arrive later, or behave differently? Explain why.

**Answer:**

### 3. Why localhost is useful
Why is `localhost` useful for testing applications even though no packets need to traverse the physical Wi-Fi network?

**Answer:**

## Part 2: Python TCP Client and Server

### 1. Client/server socket roles
Which side calls `listen()` / `accept()`, and which side calls `connect()`? Why?

**Answer:**

### 2. Information needed to connect
What two pieces of information does the client need in order to connect to the server?

**Answer:**

### 3. Loopback versus VM network address
What is the difference between `127.0.0.1` and the VM IPv4 address you found with `hostname -I` or `ip addr`?

**Answer:**

### 4. Bidirectional communication
The same TCP connection can carry data in both directions. Identify where this happens in the client/server exchange.

**Answer:**

## Part 3: ESP32-S3 TCP Client to the Same Python Server

### 1. Cross-platform interoperability
Did `tcp_server.py` need to know that the second client was an ESP32-S3 written in C rather than a Python program? Why or why not?

**Answer:**

### 2. Why the ESP32-S3 cannot use 127.0.0.1
Why can the ESP32-S3 not use `127.0.0.1` to reach the server running in the Ubuntu VM?

**Answer:**

### 3. NAT versus incoming reachability
Why can a VM in NAT mode have Internet access yet still be difficult for the ESP32-S3 to contact directly?

**Answer:**

### 4. Role of the TCP port
What role does the TCP port number play when both the ESP32-S3 and VM already have IP addresses?

**Answer:**

### 5. TCP versus the application protocol
What does TCP guarantee for the byte stream, and what does the application still have to define for itself?

**Answer:**

## Assistance acknowledgment

List all assistance received, including AI tools, people, websites, or other resources, and briefly describe what help each provided.

**Answer:**
