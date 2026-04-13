import firebase_admin
from firebase_admin import credentials, db

# Firebase initialization (run once)
cred = credentials.Certificate('serviceAccountKey.json')
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://iot-smart-park-sys-josh-default-rtdb.firebaseio.com/'
})

ref_users = db.reference('/rfid_users')
ref_slots = db.reference('/slots')

def get_user(uid):
    return ref_users.child(uid).get()

def get_all_slots():
    return ref_slots.get()

def reserve_slot(slot_id, uid):
    ref_slots.child(slot_id).update({
        'reserved': True,
        'reserved_by': uid
    })

def cancel_reservation(slot_id):
    ref_slots.child(slot_id).update({
        'reserved': False,
        'reserved_by': ''
    })
