from django.shortcuts import render, redirect
from parking.models import ParkingSlot
from django.contrib import messages
from django.views.decorators.csrf import csrf_exempt
from parking.firebase_utils import get_user, get_all_slots, reserve_slot, ref_users
from parking.forms import RFIDUserForm
from firebase_admin import db

def home(request):
    return render(request, 'parking/home.html')

def admin_dashboard_view(request):
    return render(request, 'parking/admin_dashboard.html')

def register_user_view(request):
    if request.method == 'POST':
        form = RFIDUserForm(request.POST)
        if form.is_valid():
            uid = form.cleaned_data['uid']
            name = form.cleaned_data['name']
            phone = form.cleaned_data['mobileNumber']
            ref_users.child(uid).set({
                'name': name,
                'mobileNumber' : phone
                })
            return redirect('login')  # or wherever you want to go next
    else:
        form = RFIDUserForm()
    return render(request, 'parking/register.html', {'form': form})

# views.py
@csrf_exempt
def login_view(request):
    if request.method == 'POST':
        uid_or_phone = request.POST.get('uid')
        all_users = ref_users.get()
        if all_users:
            for uid, data in all_users.items():
                if uid_or_phone == uid or uid_or_phone == data.get('mobileNumber'):
                    request.session['uid'] = uid
                    request.session['name'] = data.get('name')
                    request.session['mobileNumber'] = data.get('mobileNumber')
                    return redirect('dashboard')
        messages.error(request, 'Invalid UID or Phone Number')
    return render(request, 'parking/login.html')


def logout_view(request):
    request.session.flush()  # Clears all session data
    messages.success(request, 'You have been logged out.')
    return redirect('home')

def public_slots_view(request):
    slots = get_all_slots()
    return render(request, 'parking/slots.html', {'slots': slots})

def dashboard_view(request):
    uid = request.session.get('uid')
    name = request.session.get('name')
    if not uid:
        return redirect('login')

    slots = get_all_slots()
    return render(request, 'parking/dashboard.html', {
        'name': name,
        'uid': uid,
        'slots': slots
    })

def reserve_view(request, slot_id):
    uid = request.session.get('uid')
    if not uid:
        return redirect('login')

    reserve_slot(slot_id, uid)
    messages.success(request, f'Slot {slot_id} reserved successfully.')
    return redirect('dashboard')

"""def cancel_reservation_view(request, slot_id):
    uid = request.session.get('uid')
    user = get_user(uid)
    ref = db.reference(f"/slots/{slot_id}")
    ref.update({
        'reserved': False,
        'reserved_by': "",
        'status': "Available"
    })
    messages.success(request, f'Reservation for slot {slot_id} cancelled.')
    return redirect('dashboard')
"""
def cancel_reservation_view(request, slot_id):
    uid = request.session.get('uid')
    if not uid:
        return redirect('login')

    # Get slot info
    slot = db.reference(f"/slots/{slot_id}").get()
    
    # Only allow cancel if this user reserved the slot
    if slot and slot.get('reserved_by') == uid:
        db.reference(f"/slots/{slot_id}").update({
            'reserved': False,
            'reserved_by': '',
            'status': 'Available'
        })
        messages.success(request, f'Reservation for slot {slot_id} cancelled.')
    else:
        messages.error(request, 'You are not authorized to cancel this reservation.')

    return redirect('dashboard')
