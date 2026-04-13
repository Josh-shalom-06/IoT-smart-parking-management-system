from django import forms

class RFIDUserForm(forms.Form):
    uid = forms.CharField(label='RFID UID', max_length=20)
    name = forms.CharField(label='Name', max_length=100)
    mobileNumber = forms.CharField(label='Phone number', max_length=15)
