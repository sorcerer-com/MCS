using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

namespace MCS.Controls
{
    public class GraphsViewer : Control
    {
        #region DependencyProperty

        public static DependencyProperty DataProperty =
            DependencyProperty.Register("Data", typeof(Dictionary<string, List<Point>>), typeof(GraphsViewer));

        public static DependencyProperty LinearProperty =
            DependencyProperty.Register("Linear", typeof(bool), typeof(GraphsViewer));

        public static DependencyProperty HeaderSizeProperty =
            DependencyProperty.Register("HeaderSize", typeof(Size), typeof(GraphsViewer), new PropertyMetadata(new Size(100.0, 35.0)));

        public static DependencyProperty StartProperty =
            DependencyProperty.Register("Start", typeof(Point), typeof(GraphsViewer), new PropertyMetadata(new Point(0.0, 0.0)));

        public static DependencyProperty ScaleProperty =
            DependencyProperty.Register("Scale", typeof(Size), typeof(GraphsViewer), new PropertyMetadata(new Size(2.0, 1.0)));

        public static DependencyProperty SelectedCurveProperty =
            DependencyProperty.Register("SelectedCurve", typeof(string), typeof(GraphsViewer), new PropertyMetadata(string.Empty));

        public static DependencyProperty CurveBrushProperty =
            DependencyProperty.Register("CurveBrush", typeof(Brush), typeof(GraphsViewer), new PropertyMetadata(Brushes.MediumBlue));

        public static DependencyProperty CurveThicknessProperty =
            DependencyProperty.Register("CurveThickness", typeof(double), typeof(GraphsViewer), new PropertyMetadata(1.0));

        public static DependencyProperty KeySizeProperty =
            DependencyProperty.Register("KeySize", typeof(double), typeof(GraphsViewer), new PropertyMetadata(3.0));

        public static DependencyProperty KeyBrushProperty =
            DependencyProperty.Register("KeyBrush", typeof(Brush), typeof(GraphsViewer), new PropertyMetadata(Brushes.Brown));

        public static DependencyProperty CursorPositionProperty =
            DependencyProperty.Register("CursorPosition", typeof(double), typeof(GraphsViewer), new PropertyMetadata(0.0));

        #endregion


        public Dictionary<string, List<Point>> Data
        {
            get { return (Dictionary<string, List<Point>>)GetValue(DataProperty); }
            set { SetValue(DataProperty, value); }
        }

        public bool Linear
        {
            get { return (bool)GetValue(LinearProperty); }
            set { SetValue(LinearProperty, value); }
        }

        public Size HeaderSize
        {
            get { return (Size)GetValue(HeaderSizeProperty); }
            set { SetValue(HeaderSizeProperty, value); }
        }

        public Point Start
        {
            get { return (Point)GetValue(StartProperty); }
            set { SetValue(StartProperty, value); }
        }

        public Size Scale
        {
            get { return (Size)GetValue(ScaleProperty); }
            set { SetValue(ScaleProperty, value); }
        }

        public string SelectedCurve
        {
            get { return (string)GetValue(SelectedCurveProperty); }
            set { SetValue(SelectedCurveProperty, value); }
        }

        public Brush CurveBrush
        {
            get { return (Brush)GetValue(CurveBrushProperty); }
            set { SetValue(CurveBrushProperty, value); }
        }

        public double CurveThickness
        {
            get { return (double)GetValue(CurveThicknessProperty); }
            set { SetValue(CurveThicknessProperty, value); }
        }

        public double KeySize
        {
            get { return (double)GetValue(KeySizeProperty); }
            set { SetValue(KeySizeProperty, value); }
        }

        public Brush KeyBrush
        {
            get { return (Brush)GetValue(KeyBrushProperty); }
            set { SetValue(KeyBrushProperty, value); }
        }

        public double CursorPosition
        {
            get { return (double)GetValue(CursorPositionProperty); }
            set { SetValue(CursorPositionProperty, value); }
        }


        private Point mousePosition;

        private static Size guidelineSize = new Size(30.0, 40.0);
        private static Typeface typefaceNormal = new Typeface(new FontFamily("Arial"), FontStyles.Normal, FontWeights.Normal, FontStretches.Normal);
        private static Typeface typefaceBold = new Typeface(new FontFamily("Arial"), FontStyles.Normal, FontWeights.Bold, FontStretches.Normal);


        public GraphsViewer()
        {
            this.Background = Brushes.DarkGray;
            this.BorderBrush = Brushes.Silver;

            this.mousePosition = Mouse.GetPosition(this);
        }


        public string GetCurveName(Point pos)
        {
            pos -= (Vector)this.Start;
            pos -= (Vector)this.HeaderSize;

            int index = (int)(pos.Y / guidelineSize.Height);
            foreach (var pair in this.Data)
            {
                if (index == 0)
                    return pair.Key;

                index--;
            }
            return string.Empty;
        }


        protected override void OnPropertyChanged(DependencyPropertyChangedEventArgs e)
        {
            base.OnPropertyChanged(e);

            this.InvalidateVisual();
        }

        protected override void OnRender(DrawingContext drawingContext)
        {
            base.OnRender(drawingContext);

            if (this.Scale.Width < 0.1) this.Scale = new Size(0.1, this.Scale.Height);
            if (this.Scale.Height < 0.1) this.Scale = new Size(this.Scale.Width, 0.1);

            // snap to device pixels
            GuidelineSet guidelines = new GuidelineSet();
            guidelines.GuidelinesX.Add(0.5);
            guidelines.GuidelinesX.Add(this.ActualWidth + 0.5);
            guidelines.GuidelinesY.Add(0.5);
            guidelines.GuidelinesY.Add(this.ActualHeight + 0.5);
            drawingContext.PushGuidelineSet(guidelines);

            Pen borderPen = new Pen(this.BorderBrush, 1.0);

            /* background */
            drawingContext.DrawRectangle(this.Background, null, new Rect(0, 0, this.ActualWidth, this.ActualHeight));

            /* guidelines */
            // horizontal
            drawingContext.PushClip(new RectangleGeometry(new Rect(0, this.HeaderSize.Height, this.ActualWidth, this.ActualHeight)));
            for (double d = this.Start.Y * this.Scale.Height + this.HeaderSize.Height; d < this.ActualHeight; d += guidelineSize.Height * this.Scale.Height)
                if (d > 0.0)
                    drawingContext.DrawLine(borderPen, new Point(0, d), new Point(this.ActualWidth, d));
            drawingContext.Pop();

            // vertical
            drawingContext.PushClip(new RectangleGeometry(new Rect(this.HeaderSize.Width, 0, this.ActualWidth, this.ActualHeight)));
            double prev = this.Start.X * this.Scale.Width + this.HeaderSize.Width - 35.0;
            for (double d = this.Start.X * this.Scale.Width + this.HeaderSize.Width; d < this.ActualWidth; d += guidelineSize.Width * this.Scale.Width)
            {
                if (d > 0.0 && d - prev > 30.0)
                {
                    drawingContext.DrawLine(borderPen, new Point(d, 0), new Point(d, this.ActualHeight));
                    prev = d;
                }
            }
            drawingContext.Pop();

            // black lines bounding headers
            drawingContext.DrawLine(new Pen(this.Foreground, 1), new Point(this.HeaderSize.Width - 1, 0), new Point(this.HeaderSize.Width - 1, this.ActualHeight));
            drawingContext.DrawLine(new Pen(this.Foreground, 1), new Point(0, this.HeaderSize.Height - 1), new Point(this.ActualWidth, this.HeaderSize.Height - 1));

            /* header */
            // tracks text
            var text = new FormattedText("Tracks", System.Globalization.CultureInfo.InvariantCulture, System.Windows.FlowDirection.LeftToRight, typefaceBold, 16, this.Foreground);
            drawingContext.DrawText(text, new Point(5, this.HeaderSize.Height - text.Height - 5));

            // frames
            drawingContext.PushClip(new RectangleGeometry(new Rect(this.HeaderSize.Width, 0, this.ActualWidth, this.HeaderSize.Height)));
            int count = 0;
            prev = this.Start.X * this.Scale.Width + this.HeaderSize.Width - 35.0;
            for (double d = this.Start.X * this.Scale.Width + this.HeaderSize.Width; d < this.ActualWidth; d += guidelineSize.Width * this.Scale.Width)
            {
                text = new FormattedText(count.ToString(), System.Globalization.CultureInfo.InvariantCulture, System.Windows.FlowDirection.LeftToRight, typefaceBold, 16, this.Foreground);
                if (d > 0.0 && d - prev > 30.0)
                {
                    drawingContext.DrawText(text, new Point(d + 5, this.HeaderSize.Height - text.Height - 5));
                    prev = d;
                }
                count += (int)guidelineSize.Width;
            }
            drawingContext.Pop();

            if (this.Data != null)
            {
                // draw curves header
                drawingContext.PushClip(new RectangleGeometry(new Rect(0, this.HeaderSize.Height, this.HeaderSize.Width, this.ActualHeight)));
                count = 0;
                foreach (var pair in this.Data)
                {
                    text = new FormattedText(pair.Key, System.Globalization.CultureInfo.InvariantCulture, System.Windows.FlowDirection.LeftToRight, typefaceNormal, 16, this.Foreground);
                    if (pair.Key == this.SelectedCurve)
                        text.SetFontTypeface(typefaceBold);
                    text.MaxTextWidth = 90;
                    text.MaxLineCount = 1;
                    drawingContext.DrawText(text, new Point(5, this.Start.Y * this.Scale.Height + this.HeaderSize.Height + guidelineSize.Height * (count + 1) * this.Scale.Height - text.Height - 5));
                    count++;
                }
                drawingContext.Pop();

                // draw curves
                drawingContext.PushClip(new RectangleGeometry(new Rect(this.HeaderSize.Width, this.HeaderSize.Height, this.ActualWidth, this.ActualHeight)));
                count = 0;
                foreach (var pair in this.Data)
                {
                    Point start = new Point(this.Start.X * this.Scale.Width, this.Start.Y * this.Scale.Height);
                    start += new Vector(this.HeaderSize.Width - 1, this.HeaderSize.Height + guidelineSize.Height * count * this.Scale.Height + 1);
                    if (pair.Key == this.SelectedCurve)
                        this.drawCurve(drawingContext, start + new Vector(0, 1), guidelineSize.Height, pair.Value);

                    this.drawCurve(drawingContext, start, guidelineSize.Height, pair.Value);
                    count++;
                }
                drawingContext.Pop();
            }

            // draw cursor
            drawingContext.PushClip(new RectangleGeometry(new Rect(this.HeaderSize.Width, 0, this.ActualWidth, this.ActualHeight)));
            double cursorX = this.Start.X * this.Scale.Width + this.HeaderSize.Width + this.CursorPosition * this.Scale.Width;
            drawingContext.DrawLine(new Pen(Brushes.LimeGreen, 3), new Point(cursorX, 0), new Point(cursorX, this.HeaderSize.Height));
            drawingContext.DrawLine(new Pen(Brushes.Red, 1), new Point(cursorX, this.HeaderSize.Height), new Point(cursorX, this.ActualHeight));
            drawingContext.Pop();
        }

        private void drawCurve(DrawingContext drawingContext, Point start, double height, List<Point> data)
        {
            double minY, maxY;
            minY = data[0].Y;
            maxY = data[0].Y;
            foreach (var point in data)
            {
                minY = Math.Min(minY, point.Y);
                maxY = Math.Max(maxY, point.Y);
            }

            double scaleX = this.Scale.Width;
            double scaleY = (height * this.Scale.Height - 4.0) / (maxY - minY);
            if (double.IsInfinity(scaleY)) scaleY = 1.0;
            Point min = new Point(0, minY);

            // draw curves
            PathGeometry pg = new PathGeometry();
            PathFigure pf = new PathFigure();
            Point point0 = (Point)(data[0] - min);
            point0.X *= scaleX;
            point0.Y *= scaleY;
            point0 += (Vector)start;
            pf.StartPoint = point0 + new Vector(1, 1);
            pg.Figures.Add(pf);
            for (int i = 1; i < data.Count; i++)
            {
                Point point1 = (Point)(data[i - 1] - min);
                point1.X *= scaleX;
                point1.Y *= scaleY;
                point1 += (Vector)start;
                Point point2 = (Point)(data[i] - min);
                point2.X *= scaleX;
                point2.Y *= scaleY;
                point2 += (Vector)start;

                if (this.Linear)
                {
                    LineSegment ls = new LineSegment();
                    ls.Point = point2 + new Vector(1, 1);
                    pf.Segments.Add(ls);
                }
                else
                {
                    double d = (point2.X - point1.X) / 2.0;
                    BezierSegment bs = new BezierSegment();
                    bs.Point1 = point1 + new Vector(d, 0);
                    bs.Point2 = point2 - new Vector(d, 0);
                    bs.Point3 = point2 + new Vector(1, 1);
                    bs.IsSmoothJoin = true;
                    pf.Segments.Add(bs);
                }
            }
            drawingContext.DrawGeometry(null, new Pen(this.CurveBrush, this.CurveThickness), pg);

            // draw keys
            double keySizeDiv2 = this.KeySize / 2;
            for (int i = 0; i < data.Count; i++)
            {
                Point p = (Point)(data[i] - min);
                p.X *= scaleX;
                p.Y *= scaleY;
                p += new Vector(1, 1);
                p += (Vector)start;
                drawingContext.DrawRectangle(this.KeyBrush, null, new Rect(p.X - keySizeDiv2, p.Y - keySizeDiv2, this.KeySize, this.KeySize));

                var text = new FormattedText(data[i].Y.ToString("0.00"), System.Globalization.CultureInfo.InvariantCulture, System.Windows.FlowDirection.LeftToRight, typefaceNormal, 10, this.Foreground);
                if (i == 0)
                    drawingContext.DrawText(text, (Point)(p - new Point(-2, -2)));
                else if (data[i].Y <= data[i - 1].Y)
                    drawingContext.DrawText(text, (Point)(p - new Point(text.Width / 2, -2)));
                else
                    drawingContext.DrawText(text, (Point)(p - new Point(text.Width / 2, text.Height + 2)));
            }
        }

    }
}
