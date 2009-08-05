#include "bool.h"
#include "malloc.h"
#include "list.h"
#include "lists.h"
#include "stringBuffers.h"
#include "sockets.h"
#include "threading.h"
#include "stdlib.h"
#include "ghostlist.h"

struct member {
    struct string_buffer *nick;
    struct writer *writer;
};

/*@
predicate member(struct member* member)
    requires member->nick |-> ?nick &*& [1/2]member->writer |-> ?writer &*& string_buffer(nick) &*& writer(writer) &*& malloc_block_member(member);
@*/

struct room {
    struct list *members;
    //@ int ghost_list_id;
};

/*@
predicate room(struct room* room)
    requires room->members |-> ?membersList &*& [?f]room->ghost_list_id |-> ?id &*& foreach(?members, member) &*& list(membersList, members) &*& ghost_list(id, members) &*& malloc_block_room(room);
@*/

struct room *create_room()
    //@ requires emp;
    //@ ensures room(result);
{
    struct room *room = 0;
    struct list *members = 0;
    room = malloc(sizeof(struct room));
    if (room == 0) {
        abort();
    }
    members = create_list();
    room->members = members;
    //@ close foreach(nil, member);
    //@ int i = create_ghost_list();
    //@ room->ghost_list_id = i;
    //@ close room(room);
    return room;
}

bool room_has_member(struct room *room, struct string_buffer *nick)
    //@ requires room(room) &*& string_buffer(nick);
    //@ ensures room(room) &*& string_buffer(nick);
{
    //@ open room(room);
    //@ assert foreach(?members, _);
    struct list *membersList = room->members;
    struct iter *iter = list_create_iter(membersList);
    bool hasMember = false;
    bool hasNext = iter_has_next(iter);
    //@ length_nonnegative(members);
    while (hasNext && !hasMember)
        //@ invariant string_buffer(nick) &*& iter(iter, membersList, members, ?i) &*& foreach(members, @member) &*& hasNext == (i < length(members)) &*& 0 <= i &*& i <= length(members);
    {
        struct member *member = iter_next(iter);
        //@ mem_nth(i, members);
        //@ foreach_remove(member, members);
        //@ open member(member);
        hasMember = string_buffer_equals(member->nick, nick);
        //@ close member(member);
        //@ foreach_unremove(member, members);
        hasNext = iter_has_next(iter);
    }
    iter_dispose(iter);
    //@ close room(room);
    return hasMember;
}

void room_broadcast_message(struct room *room, struct string_buffer *message)
    //@ requires room(room) &*& string_buffer(message);
    //@ ensures room(room) &*& string_buffer(message);
{
    //@ open room(room);
    //@ assert foreach(?members0, _);
    struct list *membersList = room->members;
    struct iter *iter = list_create_iter(membersList);
    bool hasNext = iter_has_next(iter);
    //@ length_nonnegative(members0);
    while (hasNext)
        //@ invariant iter(iter, membersList, members0, ?i) &*& foreach(members0, @member) &*& string_buffer(message) &*& hasNext == (i < length(members0)) &*& 0 <= i &*& i <= length(members0);
    {
        struct member *member = iter_next(iter);
        //@ mem_nth(i, members0);
        //@ foreach_remove(member, members0);
        //@ open member(member);
        writer_write_string_buffer(member->writer, message);
        writer_write_string(member->writer, "\r\n");
        //@ close member(member);
        //@ foreach_unremove(member, members0);
        hasNext = iter_has_next(iter);
    }
    iter_dispose(iter);
    //@ close room(room);
}

struct session {
    struct room *room;
    struct lock *room_lock;
    struct socket *socket;
};

/*@

predicate_ctor room_ctor(struct room *room)()
    requires room(room);

predicate session(struct session *session)
    requires session->room |-> ?room &*& session->room_lock |-> ?roomLock &*& session->socket |-> ?socket &*& malloc_block_session(session)
        &*& [_]lock(roomLock, room_ctor(room)) &*& socket(socket, ?reader, ?writer) &*& reader(reader) &*& writer(writer);

@*/

struct session *create_session(struct room *room, struct lock *roomLock, struct socket *socket)
    //@ requires [_]lock(roomLock, room_ctor(room)) &*& socket(socket, ?reader, ?writer) &*& reader(reader) &*& writer(writer);
    //@ ensures session(result);
{
    struct session *session = malloc(sizeof(struct session));
    if (session == 0) {
        abort();
    }
    session->room = room;
    session->room_lock = roomLock;
    session->socket = socket;
    //@ close session(session);
    return session;
}

void session_run_with_nick(struct room *room, struct lock *roomLock, struct reader *reader, struct writer *writer, struct string_buffer *nick)
    //@ requires locked(roomLock, room_ctor(room), currentThread, _) &*& room(room) &*& reader(reader) &*& writer(writer) &*& string_buffer(nick);
    //@ ensures [_]lock(roomLock, room_ctor(room)) &*& reader(reader) &*& writer(writer) &*& string_buffer(nick);
{
    struct member *member = 0;

    struct string_buffer *joinMessage = create_string_buffer();
    string_buffer_append_string_buffer(joinMessage, nick);
    string_buffer_append_string(joinMessage, " has joined the room.");
    room_broadcast_message(room, joinMessage);
    string_buffer_dispose(joinMessage);

    {
        struct string_buffer *nickCopy = string_buffer_copy(nick);
        //@ open room(room);
        member = malloc(sizeof(struct member));
        if (member == 0) {
            abort();
        }
        member->nick = nickCopy;
        member->writer = writer;
        //@ split_fraction member_writer(member, _) by 1/2;
        //@ close member(member);
        list_add(room->members, member);
        //@ assert foreach(?members, @member);
        //@ close foreach(cons(member, members), @member);
        //@ assert [_]room->ghost_list_id |-> ?id;
        //@ split_fraction room_ghost_list_id(room, id);
        //@ ghost_list_add(id, member);
        //@ close room(room);
    }
    
    //@ close room_ctor(room)();
    lock_release(roomLock);
    
    {
        bool eof = false;
        struct string_buffer *message = create_string_buffer();
        while (!eof)
            //@ invariant reader(reader) &*& string_buffer(nick) &*& string_buffer(message) &*& [_]lock(roomLock, room_ctor(room));
        {
            eof = reader_read_line(reader, message);
            if (eof) {
            } else {
                lock_acquire(roomLock);
                //@ open room_ctor(room)();
                {
                    struct string_buffer *fullMessage = create_string_buffer();
                    string_buffer_append_string_buffer(fullMessage, nick);
                    string_buffer_append_string(fullMessage, " says: ");
                    string_buffer_append_string_buffer(fullMessage, message);
                    room_broadcast_message(room, fullMessage);
                    string_buffer_dispose(fullMessage);
                }
                //@ close room_ctor(room)();
                lock_release(roomLock);
            }
        }
        string_buffer_dispose(message);
    }
    
    lock_acquire(roomLock);
    //@ open room_ctor(room)();
    //@ open room(room);
    {
        struct list *membersList = room->members;
        //@ assert list(membersList, ?members);
        //@ merge_fractions room_ghost_list_id(room, _);
        //@ ghost_list_member_handle_lemma();
        list_remove(membersList, member);
        //@ foreach_remove(member, members);
    }
    //@ assert ghost_list(?id, _);
    //@ ghost_list_remove(id, member);
    //@ close room(room);
    {
        struct string_buffer *goodbyeMessage = create_string_buffer();
        string_buffer_append_string_buffer(goodbyeMessage, nick);
        string_buffer_append_string(goodbyeMessage, " left the room.");
        room_broadcast_message(room, goodbyeMessage);
        string_buffer_dispose(goodbyeMessage);
    }
    //@ close room_ctor(room)();
    lock_release(roomLock);
    
    //@ open member(member);
    string_buffer_dispose(member->nick);
    //@ merge_fractions member_writer(member, _);
    free(member);
}

/*@

predicate_family_instance thread_run_data(session_run)(void *data)
    requires session(data);

@*/

void session_run(void *data) //@ : thread_run
    //@ requires thread_run_data(session_run)(data);
    //@ ensures true;
{
    //@ open thread_run_data(session_run)(data);
    struct session *session = data;
    //@ open session(session);
    struct room *room = session->room;
    struct lock *roomLock = session->room_lock;
    struct socket *socket = session->socket;
    struct writer *writer = socket_get_writer(socket);
    struct reader *reader = socket_get_reader(socket);
    free(session);
    
    writer_write_string(writer, "Welcome to the chat room.\r\n");
    writer_write_string(writer, "The following members are present:\r\n");
    
    lock_acquire(roomLock);
    //@ open room_ctor(room)();
    //@ open room(room);
    {
        struct list *membersList = room->members;
        //@ assert list(membersList, ?members);
        struct iter *iter = list_create_iter(membersList);
        bool hasNext = iter_has_next(iter);
        //@ length_nonnegative(members);
        while (hasNext)
            //@ invariant writer(writer) &*& iter(iter, membersList, members, ?i) &*& foreach(members, @member) &*& hasNext == (i < length(members)) &*& 0 <= i &*& i <= length(members);
        {
            struct member *member = iter_next(iter);
            //@ mem_nth(i, members);
            //@ foreach_remove(member, members);
            //@ open member(member);
            writer_write_string_buffer(writer, member->nick);
            writer_write_string(writer, "\r\n");
            //@ close member(member);
            //@ foreach_unremove(member, members);
            hasNext = iter_has_next(iter);
        }
        iter_dispose(iter);
    }
    //@ close room(room);
    //@ close room_ctor(room)();
    lock_release(roomLock);

    {
        struct string_buffer *nick = create_string_buffer();
        bool done = false;
        while (!done)
          //@ invariant writer(writer) &*& reader(reader) &*& string_buffer(nick) &*& [_]lock(roomLock, room_ctor(room));
        {
            writer_write_string(writer, "Please enter your nick: ");
            {
                bool eof = reader_read_line(reader, nick);
                if (eof) {
                    done = true;
                } else {
                    lock_acquire(roomLock);
                    //@ open room_ctor(room)();
                    {
                        bool hasMember = room_has_member(room, nick);
                        if (hasMember) {
                            //@ close room_ctor(room)();
                            lock_release(roomLock);
                            writer_write_string(writer, "Error: This nick is already in use.\r\n");
                        } else {
                            session_run_with_nick(room, roomLock, reader, writer, nick);
                            done = true;
                        }
                    }
                }
            }
        }
        string_buffer_dispose(nick);
    }

    socket_close(socket);
    //@ leak [_]lock(roomLock, _);
}

int main()
    //@ requires true;
    //@ ensures false;
{
    struct room *room = create_room();
    //@ close room_ctor(room)();
    //@ close create_lock_ghost_arg(room_ctor(room));
    struct lock *roomLock = create_lock();
    struct server_socket *serverSocket = create_server_socket(12345);

    while (true)
        //@ invariant [_]lock(roomLock, room_ctor(room)) &*& server_socket(serverSocket);
    {
        struct socket *socket = server_socket_accept(serverSocket);
        //@ split_fraction lock(roomLock, _);
        struct session *session = create_session(room, roomLock, socket);
        //@ close thread_run_data(session_run)(session);
        thread_start(session_run, session);
    }
}
