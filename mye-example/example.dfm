object Form1: TForm1
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsSingle
  Caption = 'Form1'
  ClientHeight = 226
  ClientWidth = 609
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object debMemo: TMemo
    Left = 0
    Top = 0
    Width = 607
    Height = 199
    Lines.Strings = (
      'debMemo')
    ScrollBars = ssVertical
    TabOrder = 0
    Touch.ParentTabletOptions = False
    Touch.TabletOptions = [toPressAndHold]
  end
  object Button1: TButton
    Left = 3
    Top = 202
    Width = 82
    Height = 22
    Caption = 'add callback'
    TabOrder = 1
    OnClick = Button1Click
  end
  object Button2: TButton
    Left = 87
    Top = 202
    Width = 82
    Height = 22
    Caption = 'remove callback'
    TabOrder = 2
    OnClick = Button2Click
  end
end
