object Form1: TForm1
  Left = 0
  Top = 0
  Caption = 'Control panel'
  ClientHeight = 107
  ClientWidth = 290
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = True
  Position = poMainFormCenter
  PixelsPerInch = 96
  TextHeight = 13
  object Button1: TButton
    Left = 208
    Top = 37
    Width = 75
    Height = 25
    Caption = #1091#1076#1072#1083#1080#1090#1100' '
    TabOrder = 0
    OnClick = Button1Click
  end
  object Button2: TButton
    Left = 208
    Top = 8
    Width = 75
    Height = 25
    Caption = #1091#1089#1090#1072#1085#1086#1074#1080#1090#1100
    TabOrder = 1
    OnClick = Button2Click
  end
  object Button3: TButton
    Left = 84
    Top = 37
    Width = 122
    Height = 25
    Caption = #1087#1088#1086#1075#1088#1072#1084#1084#1099' '#1076#1083#1103' '#1090#1077#1089#1090#1072
    TabOrder = 2
    OnClick = Button3Click
  end
  object Button4: TButton
    Left = 8
    Top = 37
    Width = 74
    Height = 25
    Caption = 'logoff'
    TabOrder = 3
    OnClick = Button4Click
  end
  object Button5: TButton
    Left = 8
    Top = 8
    Width = 74
    Height = 25
    Caption = 'restart'
    TabOrder = 4
    OnClick = Button5Click
  end
  object Button6: TButton
    Left = 84
    Top = 8
    Width = 122
    Height = 25
    Caption = #1079#1072#1087#1091#1089#1090#1080#1090#1100' '#1101#1082#1089#1087#1083#1086#1088#1077#1088
    TabOrder = 5
    OnClick = Button6Click
  end
  object cmdEdit: TEdit
    Left = 8
    Top = 80
    Width = 121
    Height = 21
    TabOrder = 6
    Text = 'services.msc'
    OnKeyDown = cmdEditKeyDown
  end
  object Button7: TButton
    Left = 135
    Top = 80
    Width = 75
    Height = 21
    Caption = #1079#1072#1087#1091#1089#1090#1080#1090#1100
    TabOrder = 7
    OnClick = Button7Click
  end
end
