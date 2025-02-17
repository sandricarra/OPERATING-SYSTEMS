# Messaging Platform on UNIX (Operating Systems 24/25)

## General Description

This project implements a platform for sending and receiving short messages organized by topics in a UNIX environment. Messages can be persistent or non-persistent and are handled through two main programs:

- **Manager**: Coordinates message reception, distribution, and storage.
- **Feed**: Used by users to interact with the platform (sending and receiving messages).

## Functionality

### 1. Involved Programs

#### **Manager**

- Acts as the central message server.
- Manages the list of users and available topics.
- Distributes sent messages to the corresponding subscribers.
- Stores persistent messages and retrieves those that have not expired upon restart.
- Allows administrative commands such as listing users, removing users, and blocking topics.

#### **Feed**

- Serves as the user interface in the console.
- Allows users to identify themselves with a unique name.
- Sends and receives messages in real time.
- Facilitates subscription and unsubscription from topics.

### 2. Message Types

- **Non-Persistent**: Sent and delivered immediately to subscribers without being stored.
- **Persistent**: Stored and remain available until they expire according to the sender-defined lifespan.

### 3. Defined Limits

- **Maximum users**: 10
- **Maximum topics**: 20
- **Persistent messages per topic**: 5

## Available Commands

### **Feed** (Client User)

| Command                              | Description                                                                          |
| ------------------------------------ | ------------------------------------------------------------------------------------ |
| `./feed <username>`                 | Starts the user program with the specified name.                                    |
| `topics`                             | Lists available topics and their details.                                           |
| `msg <topic> <duration> <message>`   | Sends a message to a topic with a defined duration (0 for non-persistent).          |
| `subscribe <topic>`                  | Subscribes to a topic and receives its messages.                                    |
| `unsubscribe <topic>`                | Unsubscribes from a topic.                                                          |
| `exit`                               | Closes the Feed program.                                                            |

### **Manager** (Administrator)

| Command           | Description                                               |
| ----------------- | --------------------------------------------------------- |
| `list_users`      | Displays the list of active users.                        |
| `list_topics`     | Lists existing topics and their status.                   |
| `kick <username>` | Removes a user from the platform.                         |
| `block <topic>`   | Blocks a topic (new messages cannot be sent).             |
| `unblock <topic>` | Unblocks a topic.                                         |
| `shutdown`        | Shuts down the manager, saving persistent messages.       |

## Persistent Message Storage

- Stored in a text file defined by the environment variable `MSG_FICH`.
- Each line follows the format:
  ```
  <topic> <username> <remaining lifespan> <message>
  ```
- Example:
  ```
  football sr_silva 110 that game for the league was very strange
  snacks mario 99 does anyone know where to buy cheap bifanas?
  ```

## Final Considerations

- The **manager** must be active for the **feed** programs to function properly.
- The use of **system calls** (e.g., `read()`, `write()`) is recommended instead of library functions (e.g., `fread()`, `fwrite()`).
- Commands should provide **confirmation messages** to improve user experience.
- The **administrator** is not the root user of the operating system, only the manager.

---

